#pragma once
#include <UnigineNodes.h>
#include <UnigineObjects.h>
#include <UnigineMathLib.h>

namespace SkeletonIK
{

Unigine::Math::mat4 jointObjectTransform(const Unigine::NodeSkeletonPosePtr &pose, int joint);

void setJointLocalRotation(const Unigine::NodeSkeletonPosePtr &pose, int joint, const Unigine::Math::quat &localRot);

// Write a joint's desired WORLD rotation, converting it through the joint's parent
// (which must be unchanged this frame). Keeps the joint's local translation.
void setJointWorldRotation(const Unigine::NodeSkeletonPosePtr &pose,
	const Unigine::ObjectMeshSkinnedPtr &skinned, int joint, const Unigine::Math::quat &worldRot);

void solveTwoBone(
	const Unigine::Math::Vec3 &hip, const Unigine::Math::Vec3 &knee, const Unigine::Math::Vec3 &foot,
	const Unigine::Math::Vec3 &target, const Unigine::Math::vec3 &pole, float l1, float l2,
	const Unigine::Math::quat &hipRot, const Unigine::Math::quat &kneeRot,
	Unigine::Math::quat &outHipRot, Unigine::Math::quat &outKneeRot);

Unigine::Math::vec3 axisVec(int s);

} // namespace SkeletonIK
