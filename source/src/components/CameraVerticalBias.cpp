#include "CameraVerticalBias.h"

using namespace Unigine;
using namespace Unigine::Math;

static inline float powf_safe(float x, float p)
{
	return (p <= 1.0f) ? x : powf(x, p);
}

void CameraVerticalBias::init()
{
	_bias = 0.0f;
}

Vec3 CameraVerticalBias::update(float pitchDeg,
	const vec2 &pitchRange,
	const vec3 &biasForward,
	const Vec3 &rawTarget,
	float dt)
{
	float wMin = 0.0f;
	float wMax = 0.0f;

	const float zoneDeg = min_angle_bias_offset;

	if (enable_min_angle_bias && zoneDeg > 0.0f)
	{
		float t = (pitchDeg - pitchRange.x) / zoneDeg;
		float k = 1.0f - saturate(t);
		wMin = powf_safe(k, bias_power);
	}

	if (enable_max_angle_bias && zoneDeg > 0.0f)
	{
		float t = (pitchRange.y - pitchDeg) / zoneDeg;
		float k = 1.0f - saturate(t);
		wMax = powf_safe(k, bias_power);
	}

	_bias = _bias + (wMin - wMax - _bias) * (1.0f - expf(-bias_smooth * dt));

	float shift = (_bias >= 0.0f ? min_shift_units : max_shift_units) * _bias;

	if (shift == 0.0f)
		return rawTarget;

	return rawTarget + normalize(Vec3(biasForward)) * shift;
}
