#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraRotationLagRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraRotationLagRig, CameraStageModifier)

	PROP_PARAM(Toggle, only_when_no_input, true)
	PROP_PARAM(Toggle, separate_axis, false)

	PROP_PARAM(Float, speed, 12.0f, "", "", "", "separate_axis=0")
	PROP_PARAM(Vec2, speed_axis, Unigine::Math::vec2(12.0f), "", "", "", "separate_axis=1")

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

	void init(const Unigine::Math::vec2 &rawAngle);
	Unigine::Math::vec2 update(const Unigine::Math::vec2 &rawAngle, float ifps);

private:
	Unigine::Math::vec2 _angle;
	Unigine::Math::vec2 _velocity;

	Unigine::Math::vec2 _omega;
};
