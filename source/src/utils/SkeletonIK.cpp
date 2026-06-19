#include "utils/SkeletonIK.h"

#include <UnigineSkeleton.h>

using namespace Unigine;
using namespace Unigine::Math;

namespace SkeletonIK
{
mat4 jointObjectTransform(const NodeSkeletonPosePtr &pose, int joint)
{
	mat4 m = pose->getJointTransform(joint);
	auto skeleton = pose->getSkeleton();
	for (int p = skeleton->getJointParent(joint); p != -1; p = skeleton->getJointParent(p))
	{
		m = pose->getJointTransform(p) * m;
	}
	return m;
}

void setJointLocalRotation(const NodeSkeletonPosePtr &pose, int joint, const quat &localRot)
{
	pose->setJointTransform(joint, mat4(localRot, (pose->getJointTransform(joint)).getTranslate()));
}

void setJointWorldRotation(const NodeSkeletonPosePtr &pose, const ObjectMeshSkinnedPtr &skinned, int joint, const quat &worldRot)
{
	int parent = pose->getSkeleton()->getJointParent(joint);
	quat sw_rot = skinned->getWorldTransform().getRotate();
	quat parent_world = (parent >= 0) ? sw_rot * jointObjectTransform(pose, parent).getRotate() : sw_rot;
	setJointLocalRotation(pose, joint, inverse(parent_world) * worldRot);
}

void solveTwoBone(const Vec3 &hip, const Vec3 &knee, const Vec3 &foot, const Vec3 &target,
	const vec3 &pole, float l1, float l2, const quat &hipRot, const quat &kneeRot,
	quat &outHipRot, quat &outKneeRot)
{
	vec3 to_target = vec3(target - hip);
	float d = length(to_target);
	float dmin = abs(l1 - l2) + 1e-3f;
	float dmax = l1 + l2 - 1e-3f;
	d = clamp(d, dmin, dmax);
	vec3 axis = (length(to_target) > Consts::EPS) ? normalize(to_target) : normalize(vec3(knee - hip));

	vec3 bend = pole - axis * dot(pole, axis);
	if (length(bend) < Consts::EPS)
		bend = vec3_up - axis * dot(vec3_up, axis);
	if (length(bend) < Consts::EPS)
		bend = vec3_forward - axis * dot(vec3_forward, axis);
	bend = normalize(bend);
	float cos_a = clamp((l1 * l1 + d * d - l2 * l2) / (2.0f * l1 * d), -1.0f, 1.0f);
	float sin_a = sqrt(max(0.0f, 1.0f - cos_a * cos_a));
	Vec3 knee_new = hip + Vec3(axis * (l1 * cos_a) + bend * (l1 * sin_a));
	Vec3 target_c = hip + Vec3(axis * d);

	quat hip_delta = rotationFromTo(normalize(vec3(knee - hip)), normalize(vec3(knee_new - hip)));
	outHipRot = hip_delta * hipRot;

	vec3 lower_after_hip = hip_delta * normalize(vec3(foot - knee));
	vec3 lower_target = normalize(vec3(target_c - knee_new));
	quat knee_delta = rotationFromTo(lower_after_hip, lower_target);
	outKneeRot = knee_delta * (hip_delta * kneeRot);
}

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

} // namespace SkeletonIK
