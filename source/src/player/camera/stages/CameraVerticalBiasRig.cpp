#include "CameraVerticalBiasRig.h"

REGISTER_COMPONENT(CameraVerticalBiasRig)

using namespace Unigine;
using namespace Unigine::Math;

static inline float powf_safe(float x, float p)
{
	return (p <= 1.0f) ? x : powf(x, p);
}

void CameraVerticalBiasRig::runtimeReset(CameraState &state, const CameraContext &)
{
	_bias = 0.0f;
	_velWeight = 1.0f;
}

void CameraVerticalBiasRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	const float dt = state.dt;

	const bool hasInput =
		abs(input.angle.x) > 1e-4f ||
		abs(input.angle.y) > 1e-4f ||
		abs(input.scroll)  > 1e-6f;

	const float speed = (float)ctx.targetHorizontalSpeed;
	const float vth = max(velocity_treshold.get(), 0.0f);

	float velWeightTarget = 1.0f;
	if (vth > 1e-4f)
	{
		float t = saturate(speed / vth);
		float k = 1.0f - t;
		velWeightTarget = k * k * (3.0f - 2.0f * k);
	}
	else
	{
		velWeightTarget = 0.0f;
	}

	if (hasInput)
	{
		velWeightTarget = _velWeight;
	}

	_velWeight = _velWeight + (velWeightTarget - _velWeight) * (1.0f - expf(-smooth.get() * dt));

	if (_velWeight < 1e-4f)
	{
		_bias = _bias + (0.0f - _bias) * (1.0f - expf(-smooth.get() * dt));
		return;
	}

	const float pitch = state.rig.angle.y;
	const vec2 pr = state.rig.pitch_range;

	float wMin = 0.0f;
	float wMax = 0.0f;

	const float z = max(zone_deg.get(), 0.0f);
	if (z > 1e-4f)
	{
		if (enable_min_angle_bias.get())
		{
			float t = (pitch - pr.x) / z;
			float k = 1.0f - saturate(t);
			wMin = powf_safe(k, bias_power.get());
		}

		if (enable_max_angle_bias.get())
		{
			float t = (pr.y - pitch) / z;
			float k = 1.0f - saturate(t);
			wMax = powf_safe(k, bias_power.get());
		}
	}

	float targetBias = (wMin - wMax);

	targetBias *= _velWeight;

	_bias = _bias + (targetBias - _bias) * (1.0f - expf(-smooth.get() * dt));

	float shift = (_bias >= 0.0f ? min_shift_units.get() : max_shift_units.get()) * _bias;
	if (abs(shift) < 1e-5f)
		return;

	quat qYaw = quat(vec3_up, state.rig.angle.x);
	quat qOrbit = quat(qYaw * vec3_right, state.rig.angle.y) * qYaw;
	Vec3 forward = Vec3(qOrbit * vec3_forward);

	forward.z = 0.0f;
	float l2 = length2(forward);
	if (l2 < 1e-6f)
		return;

	forward /= sqrtf(l2);
	forward = normalize(forward);

	state.rig.pivot = state.rig.pivot + Vec3(forward * shift);
}


