#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraAutoAlignYawRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraAutoAlignYawRig, CameraStageModifier);

	PROP_PARAM(Float, input_deadzone, 0.05f)
	PROP_PARAM(Float, delay, 3.0f)

	PROP_PARAM(Float, speed, 10.0f)
	PROP_PARAM(Float, max_deg_per_sec, 180.0f)

	PROP_PARAM(Float, yaw_offset_deg, 0.0f)

	PROP_PARAM(Float, vel_dir_smooth, 12.0f)
	PROP_PARAM(Float, target_yaw_smooth, 10.0f)

	void runtimeReset(CameraState &, const CameraContext &) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	float _noInputTime = 0.0f;
	Unigine::Math::Vec3 _lastPos;
	bool _hasLastPos = false;

	bool _alignActive = false;
	Unigine::Math::Vec3 _velLP;

	float _latchedTargetYaw = 0.0f;
	bool _latched = false;
	Unigine::Math::Vec3 _velDirSmoothed = Unigine::Math::Vec3(0, 1, 0);
};
