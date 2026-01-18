#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraVerticalBiasRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraVerticalBiasRig, CameraStageModifier);

	PROP_PARAM(Toggle, enable_min_angle_bias, 1);
	PROP_PARAM(Toggle, enable_max_angle_bias, 1);

	PROP_PARAM(Float, zone_deg, 12.0f);

	PROP_PARAM(Float, min_shift_units, 5.0f);
	PROP_PARAM(Float, max_shift_units, 5.0f);

	PROP_PARAM(Float, bias_power, 1.5f);

	PROP_PARAM(Float, smooth, 3.0f);

	void runtimeReset(CameraState &state, const CameraContext &) override;
	void apply(CameraState &state, const CameraInput &, const CameraContext &) override;

private:
	float _bias = 0.0f;
};
