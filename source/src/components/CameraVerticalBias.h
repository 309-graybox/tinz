#pragma once
#include <UnigineComponentSystem.h>

struct CameraVerticalBias final: public Unigine::ComponentStruct
{
	PROP_PARAM(Toggle, enable_min_angle_bias, true)
	PROP_PARAM(Float, min_angle_bias_offset, 45.0f)

	PROP_PARAM(Toggle, enable_max_angle_bias, true)

	PROP_PARAM(Float, min_shift_units, 2.0f)
	PROP_PARAM(Float, max_shift_units, 2.0f)

	PROP_PARAM(Float, bias_power, 2.0f)
	PROP_PARAM(Float, bias_smooth, 20.0f)

	void init();
	Unigine::Math::Vec3 update(float pitch,
		const Unigine::Math::vec2 &pitchRange,
		const Unigine::Math::vec3 &biasForward,
		const Unigine::Math::Vec3 &rawTarget,
		float ifps);

private:
	float _bias = 0.0f;
};
