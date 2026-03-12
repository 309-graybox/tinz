#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraAutoAlignPitchRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraAutoAlignPitchRig, CameraStageModifier);

	PROP_PARAM(Float, input_deadzone, 0.05f)
	PROP_PARAM(Float, delay, 3.0f)

	PROP_PARAM(Float, speed, 10.0f)
	PROP_PARAM(Float, max_deg_per_sec, 180.0f)

	PROP_PARAM(Float, pitch_offset_deg, 0.0f)

	PROP_PARAM(Float, vel_dir_smooth, 12.0f)
	PROP_PARAM(Float, target_pitch_smooth, 10.0f)

	// Если нужно ограничить pitch, чтобы не улетал в зенит/надир
	PROP_PARAM(Float, min_pitch_deg, -89.0f)
	PROP_PARAM(Float, max_pitch_deg, 89.0f)

	// Диапазон, к которому автовыравнивание тянет pitch (внутри min/max_pitch_deg)
	PROP_PARAM(Float, align_min_deg, -10.0f)
	PROP_PARAM(Float, align_max_deg, 10.0f)

	PROP_PARAM(Toggle, reset_timer_on_stop, true)
	PROP_PARAM(Float, min_target_velocity, 2.0f)

	void runtimeReset(CameraState &, const CameraContext &) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	float _noInputTime = 0.0f;
	float _latchedTargetPitch = 0.0f;
	bool _latched = false;

	Unigine::Math::Vec3 _velDirSmoothed = Unigine::Math::Vec3_forward;
};
