#pragma once
#include <UnigineMathLib.h>

struct CameraRigState
{
	Unigine::Math::Vec3 pivot;

	Unigine::Math::vec2 angle;

	double arm_len = 3.0;

	Unigine::Math::dvec3 local_offset;

	Unigine::Math::vec2 pitch_range = {-1.2f, 1.0f};
	Unigine::Math::dvec2 arm_range = {1.2f, 5.0f};
};

struct CameraImpulses
{
	Unigine::Math::Vec3 pos_kick;
	Unigine::Math::Vec2 rot_kick;
	float fov_kick = 0.0f;
};

struct CameraState
{
	CameraRigState rig;

	Unigine::Math::Vec3 desired_pos;
	Unigine::Math::quat desired_rot;

	Unigine::Math::Vec3 pos;
	Unigine::Math::quat rot;

	float fov = 60.0f;
	float dt = 1.0f;

	CameraImpulses impulses;
};
