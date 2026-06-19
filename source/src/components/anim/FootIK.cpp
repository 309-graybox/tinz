#include "components/anim/FootIK.h"

#include "player/movement/CharacterMovement.h"
#include "utils/SkeletonIK.h"

#include <UnigineGame.h>
#include <UnigineLog.h>
#include <UnigineSkeleton.h>
#include <UnigineWorld.h>

REGISTER_COMPONENT(FootIK)

using namespace Unigine;
using namespace Unigine::Math;

void FootIK::init()
{
	_pose = checked_ptr_cast<NodeSkeletonPose>(node);
	COMPONENT_REQUIRE(_pose, String::format("FootIK \"%s\": node is not a NodeSkeletonPose\n", node->getName()).get(), return);

	auto skeleton = _pose->getSkeleton();
	COMPONENT_REQUIRE(skeleton, "FootIK: NodeSkeletonPose has no skeleton\n", return);

	_skinned = _pose->getControlledObject(0);
	COMPONENT_REQUIRE(_skinned, "FootIK: NodeSkeletonPose has no controlled skinned mesh\n", return);

	String pfx = jointPrefix.get();
	_hips = skeleton->findJoint((pfx + hipsJoint));
	COMPONENT_REQUIRE(_hips != -1, "FootIK: No hips found\n", return);

	resolveLeg(_left, leftUpLeg, leftKnee, leftFoot);
	COMPONENT_REQUIRE(_left.upleg != -1, "FootIK: No left upleg found\n", return);
	COMPONENT_REQUIRE(_left.knee != -1, "FootIK: No left knee found\n", return);
	COMPONENT_REQUIRE(_left.foot != -1, "FootIK: No left foot found\n", return);

	resolveLeg(_right, rightUpLeg, rightKnee, rightFoot);
	COMPONENT_REQUIRE(_right.upleg != -1, "FootIK: No right upleg found\n", return);
	COMPONENT_REQUIRE(_right.knee != -1, "FootIK: No right knee found\n", return);
	COMPONENT_REQUIRE(_right.foot != -1, "FootIK: No right foot found\n", return);

	_movement = getComponent<CharacterMovement>(movement);
}

void FootIK::postUpdate()
{
	if (!_pose || !_skinned)
		return;

	float ifps = Game::getIFps();
	if (ifps <= 0.0f)
		return;

	vec3 up = vec3_up;
	float ts = saturate(1.0f - exp(-(float)turnSpeed * ifps));
	bool active = !_movement || (_movement->isGrounded() && stateAllowed());

	Probe pl, pr;
	if (_left.valid())
		pl = probeLeg(_left, up);
	if (_right.valid())
		pr = probeLeg(_right, up);

	float want_drop = 0.0f;
	if (active)
	{
		float min_lift = 0.0f;
		if (pl.grounded)
			min_lift = min(min_lift, pl.lift);
		if (pr.grounded)
			min_lift = min(min_lift, pr.lift);
		want_drop = clamp(-min_lift, 0.0f, (float)maxPelvisDrop);
	}

	_pelvis_smoothed = Math::lerp(_pelvis_smoothed, want_drop, ts);
	if (_hips >= 0 && _pelvis_smoothed > Consts::EPS)
		applyPelvisDrop(_pelvis_smoothed, up);

	if (_left.valid())
	{
		const float w = (active && pl.grounded) ? saturate((float)weight) : 0.0f;
		solveLeg(_left, pl.target, w, ts);
	}
	if (_right.valid())
	{
		const float w = (active && pr.grounded) ? saturate((float)weight) : 0.0f;
		solveLeg(_right, pr.target, w, ts);
	}

	_pose->forceApplyPose();
}

void FootIK::resolveLeg(Leg &leg, const char *upleg, const char *knee, const char *foot)
{
	auto skeleton = _pose->getSkeleton();
	String pfx = jointPrefix.get();
	leg.upleg = skeleton->findJoint((pfx + upleg).get());
	leg.knee = skeleton->findJoint((pfx + knee).get());
	leg.foot = skeleton->findJoint((pfx + foot).get());
	if (!leg.valid())
		return;

	vec3 hip = SkeletonIK::jointObjectTransform(_pose, leg.upleg).getColumn3(3);
	vec3 kn = SkeletonIK::jointObjectTransform(_pose, leg.knee).getColumn3(3);
	vec3 ft = SkeletonIK::jointObjectTransform(_pose, leg.foot).getColumn3(3);
	leg.l1 = length(kn - hip);
	leg.l2 = length(ft - kn);
}

bool FootIK::stateAllowed() const
{
	switch (_movement->getMovementState())
	{
		case IDLE: return activeInIdle;
		case MOVE: return activeInMove;
		case SLIDE: return activeInSlide;
		default: return false;
	}
}

FootIK::Probe FootIK::probeLeg(const Leg &leg, const vec3 &up) const
{
	Probe pr;
	mat4 foot_obj = SkeletonIK::jointObjectTransform(_pose, leg.foot);
	Mat4 sw = _skinned->getWorldTransform();
	pr.foot_p = (sw * Mat4(foot_obj)).getColumn3(3);

	Vec3 from = pr.foot_p + Vec3(up) * Scalar(traceUp);
	Vec3 to = pr.foot_p - Vec3(up) * Scalar(traceDown);
	WorldIntersectionNormalPtr hit = WorldIntersectionNormal::create();
	ObjectPtr ground = World::getIntersection(from, to, groundMask.get(), hit);
	if (ground)
	{
		pr.grounded = true;
		pr.target = hit->getPoint() + Vec3(up) * Scalar(footHeight);
		pr.lift = static_cast<float>(dot(vec3(pr.target - pr.foot_p), up));
	} else
		pr.target = pr.foot_p;
	return pr;
}

void FootIK::applyPelvisDrop(float drop, const vec3 &up)
{
	mat4 hips_local = _pose->getJointTransform(_hips);
	int parent = _pose->getSkeleton()->getJointParent(_hips);
	quat sw_rot = _skinned->getWorldTransform().getRotate();
	quat parent_world = (parent >= 0) ? sw_rot * SkeletonIK::jointObjectTransform(_pose, parent).getRotate() : sw_rot;

	vec3 disp_local = inverse(parent_world) * (-up * drop);
	_pose->setJointTransform(_hips, mat4(hips_local.getRotate(), hips_local.getTranslate() + disp_local));
}

void FootIK::solveLeg(Leg &leg, const Vec3 &target, float w, float ts)
{
	mat4 upleg_obj = SkeletonIK::jointObjectTransform(_pose, leg.upleg);
	mat4 knee_obj = SkeletonIK::jointObjectTransform(_pose, leg.knee);
	mat4 foot_obj = SkeletonIK::jointObjectTransform(_pose, leg.foot);

	Mat4 sw = _skinned->getWorldTransform();
	quat sw_rot = sw.getRotate();
	Vec3 hip_p = (sw * Mat4(upleg_obj)).getColumn3(3);
	Vec3 knee_p = (sw * Mat4(knee_obj)).getColumn3(3);
	Vec3 foot_p = (sw * Mat4(foot_obj)).getColumn3(3);
	quat hip_w = sw_rot * upleg_obj.getRotate();
	quat knee_w = sw_rot * knee_obj.getRotate();

	vec3 pole = normalize(sw_rot * SkeletonIK::axisVec(forwardAxis));
	quat out_hip = hip_w;
	quat out_knee = knee_w;
	if (w > Consts::EPS)
		SkeletonIK::solveTwoBone(hip_p, knee_p, foot_p, target, pole, leg.l1, leg.l2, hip_w, knee_w, out_hip, out_knee);

	quat want_hip = slerp(hip_w, out_hip, w);
	quat want_knee = slerp(knee_w, out_knee, w);
	if (!leg.smoothed_init)
	{
		leg.smoothed_hip = hip_w;
		leg.smoothed_knee = knee_w;
		leg.smoothed_init = true;
	}
	leg.smoothed_hip = slerp(leg.smoothed_hip, want_hip, ts);
	leg.smoothed_knee = slerp(leg.smoothed_knee, want_knee, ts);

	SkeletonIK::setJointWorldRotation(_pose, _skinned, leg.upleg, leg.smoothed_hip);
	SkeletonIK::setJointLocalRotation(_pose, leg.knee, inverse(leg.smoothed_hip) * leg.smoothed_knee);
}
