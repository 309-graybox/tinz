#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>
#include <UnigineNodes.h>
#include <UnigineObjects.h>

class HeadLookAt: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(HeadLookAt, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_POST_UPDATE(postUpdate)

	PROPERTY_ND(Node, target, Tooltip("Target the character turns its head toward"))
	PROPERTY_ND(Node, skeletonPose, Tooltip("NodeSkeletonPose node (defaults to this node if empty)"))
	PROPERTY(String, headJoint, "head", Tooltip("Head joint name in the skeleton"))
	PROPERTY_SWITCH(lookAxis, 3, "-Z,Z,-Y,Y,-X,X", Tooltip("Head-LOCAL axis that points out of the face (the gaze). Enable debugDraw and pick the colored arrow that points forward out of the head: X=red, Y=green, Z=blue"))
	PROPERTY(Float, weight, 1.0f, Tooltip("Head look-at strength [0..1]"))
	PROPERTY(Float, maxYaw, 70.0f, Tooltip("Max horizontal turn (left/right) from the rest gaze, degrees"))
	PROPERTY(Float, maxPitch, 45.0f, Tooltip("Max vertical turn (up/down) from the rest gaze, degrees"))
	PROPERTY(Float, turnSpeed, 12.0f, Tooltip("Exponential smoothing speed of the turn (1/s)"))

private:
	void init();
	void postUpdate();
	void resolve();

private:
	Unigine::NodeSkeletonPosePtr _pose;
	Unigine::ObjectMeshSkinnedPtr _skinned;
	int _joint = -1;
	bool _resolved = false;

	Unigine::Math::quat _rest_obj_rot;
	Unigine::Math::quat _smoothed;
};
