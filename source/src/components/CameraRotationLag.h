#pragma once
#include <UnigineComponentSystem.h>

struct CameraRotationLag final: public Unigine::ComponentStruct
{
	PROP_PARAM(Toggle, separate_axis, false)

	PROP_PARAM(Float, speed, 20.0f, "", "", "", "separate_axis=0")
	PROP_PARAM(Vec2, speed_axis, Unigine::Math::vec2(20.0f), "", "", "", "separate_axis=1")

	void init(const Unigine::Math::vec2 &rawAngle);
	Unigine::Math::vec2 update(const Unigine::Math::vec2 &rawAngle, float ifps);

private:
	Unigine::Math::vec2 _angle;
	Unigine::Math::vec2 _velocity;

	Unigine::Math::vec2 _omega;
};
