#include "components/anim/HeadLookAt.h"

#include <UnigineGame.h>
#include <UnigineSkeleton.h>
#include <UnigineVisualizer.h>

REGISTER_COMPONENT(HeadLookAt)

using namespace Unigine;
using namespace Unigine::Math;

namespace
{

vec3 axisVec(int s)
{
	switch (s)
	{
		case 0: return vec3_down;
		case 1: return vec3_up;
		case 2: return vec3_back;
		case 3: return vec3_forward;
		case 4: return vec3_left;
		default: return vec3_right;
	}
}

} // namespace

void HeadLookAt::init()
{
	resolve();
}

void HeadLookAt::resolve()
{
	NodePtr pose_node = skeletonPose.get() ? skeletonPose.get() : node;
	_pose = checked_ptr_cast<NodeSkeletonPose>(pose_node);
	COMPONENT_REQUIRE(_pose, String::format("HeadLookAt \"%s\": node \"%s\" is not a NodeSkeletonPose\n", node->getName(), pose_node ? pose_node->getName() : "<null>").get(), return);

	auto skeleton = _pose->getSkeleton();
	COMPONENT_REQUIRE(skeleton, "HeadLookAt: NodeSkeletonPose has no skeleton\n", return);

	_joint = skeleton->findJoint(headJoint.get());
	COMPONENT_REQUIRE(_joint >= 0, String::format("HeadLookAt \"%s\": joint \"%s\" not found in skeleton\n", node->getName(), headJoint.get()).get(), return);

	_skinned = _pose->getControlledObject(0);
	COMPONENT_REQUIRE(_skinned, "HeadLookAt: NodeSkeletonPose has no controlled skinned mesh\n", return);

	quat head_obj_rot = jointObjectTransform(_joint).getRotate();
	_rest_obj_rot = head_obj_rot;
	_smoothed = head_obj_rot;
}

mat4 HeadLookAt::jointObjectTransform(int joint) const
{
	mat4 m = _pose->getJointTransform(joint);
	auto skeleton = _pose->getSkeleton();
	for (int p = skeleton->getJointParent(joint); p != -1; p = skeleton->getJointParent(p))
	{
		m = _pose->getJointTransform(p) * m;
	}
	return m;
}

void HeadLookAt::postUpdate()
{
	if (!_pose || !_skinned || _joint < 0 || !target)
		return;

	float ifps = Game::getIFps();
	if (ifps <= 0.0f)
		return;

	mat4 head_local = _pose->getJointTransform(_joint);
	mat4 head_obj = jointObjectTransform(_joint);
	quat head_obj_rot = head_obj.getRotate();
	quat parent_obj_rot = head_obj_rot * inverse(head_local.getRotate());

	Mat4 sw = _skinned->getWorldTransform();
	quat sw_rot = sw.getRotate();
	Mat4 head_world = sw * Mat4(head_obj);
	Vec3 head_pos = head_world.getColumn3(3);

	vec3 gaze_local = axisVec(lookAxis);
	vec3 rest_gaze_w = normalize((sw_rot * _rest_obj_rot) * gaze_local);

	vec3 to_tgt = vec3(target->getWorldPosition() - head_pos);

	if (length(to_tgt) < Consts::EPS)
		return;

	vec3 dir = normalize(to_tgt);

	vec3 up = vec3_up;
	vec3 rest_h = rest_gaze_w - up * dot(rest_gaze_w, up);
	vec3 dir_h = dir - up * dot(dir, up);
	if (length(dir_h) < Consts::EPS)
		dir_h = rest_h;

	rest_h = normalize(rest_h);
	dir_h = normalize(dir_h);

	float yaw = clamp(getAngle(rest_h, dir_h, up), -(float)maxYaw, (float)maxYaw);
	float elev_tgt = asin(clamp(dot(dir, up), -1.0f, 1.0f)) * Consts::RAD2DEG;
	float elev_rest = asin(clamp(dot(rest_gaze_w, up), -1.0f, 1.0f)) * Consts::RAD2DEG;
	float pitch = clamp(elev_tgt - elev_rest, -(float)maxPitch, (float)maxPitch);

	quat rest_world = sw_rot * _rest_obj_rot;
	quat want_world = quat(cross(dir_h, up), pitch) * quat(up, yaw) * rest_world;
	quat want_obj = inverse(sw_rot) * want_world;

	quat target_obj = slerp(head_obj_rot, want_obj, saturate((float)weight));

	float ts = saturate(1.0f - exp(-(float)turnSpeed * ifps));
	_smoothed = slerp(_smoothed, target_obj, ts);

	quat local_rot = inverse(parent_obj_rot) * _smoothed;
	_pose->setJointTransform(_joint, mat4(local_rot, head_local.getTranslate()));
	_pose->forceApplyPose();
}
