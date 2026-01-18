#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraAutoAlignYawRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraAutoAlignYawRig, CameraStageModifier)

	PROP_PARAM(Float, input_deadzone, 0.05f)
	PROP_PARAM(Float, delay, 2.0f)
	PROP_PARAM(Float, speed, 6.0f)
	PROP_PARAM(Float, max_deg_per_sec, 180.0f)
	PROP_PARAM(Toggle, require_target_velocity, 0)
	PROP_PARAM(Float, min_target_speed, 0.2f)
	PROP_PARAM(Float, yaw_offset_deg, 0.0f)

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	float approach_yaw(float current, float target, float dt) const;

private:
	float _noInputTime = 0.0f;
	Unigine::Math::Vec3 _lastTargetPos;
};
