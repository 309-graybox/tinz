#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraOrbitIntent final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraOrbitIntent, CameraStageModifier)

	PROP_PARAM(DVec3, pivot_offset, {0, 0, 1.6f});

	PROP_PARAM(Vec2, angle_mul, {0.5f, 0.5f});

	PROP_PARAM(Double, zoom_speed, 0.25)
	PROP_PARAM(Double, default_arm, 3.0)
	PROP_PARAM(Vec2, pitch_range, {-89.9f, 89.9f})
	PROP_PARAM(DVec2, arm_range, {1.2, 5.0})

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;
};
