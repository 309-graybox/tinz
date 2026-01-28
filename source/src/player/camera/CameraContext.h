#pragma once
#include <UnigineNode.h>

class PlayerCameraManager;

class CameraContext
{
public:
	Unigine::NodePtr target;
	Unigine::Math::Vec3 targetVelocity;
	Unigine::Math::Vec3 targetSpeedDir;
	Unigine::Math::Scalar targetSpeed;
	Unigine::Math::Vec3 targetHorizontalVelocity;
	Unigine::Math::Vec3 targetHorizontalSpeedDir;
	Unigine::Math::Scalar targetHorizontalSpeed;
	Unigine::Math::Vec3 targetVerticalVelocity;
	Unigine::Math::Vec3 targetVerticalSpeedDir;
	Unigine::Math::Scalar targetVerticalSpeed;
	Unigine::NodePtr cameraNode;

	// TODO WTF???
	mutable int collision_mask = 0xffffffff;
};
