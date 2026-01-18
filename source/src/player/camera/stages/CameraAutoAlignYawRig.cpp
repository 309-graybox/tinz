#include "CameraAutoAlignYawRig.h"

REGISTER_COMPONENT(CameraAutoAlignYawRig)

using namespace Unigine;
using namespace Unigine::Math;

static inline float normalize_deg(float a)
{
	while (a >= 180.0f)
		a -= 360.0f;
	while (a < -180.0f)
		a += 360.0f;
	return a;
}

void CameraAutoAlignYawRig::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	_noInputTime = 0.0f;
	_lastTargetPos = Vec3_zero;
}

void CameraAutoAlignYawRig::apply(CameraState &state, const CameraInput &input,
	const CameraContext &ctx)
{
	float dt = state.dt;
	if (dt <= 0.0f)
		dt = 1.0f / 60.0f;

	const bool has_input = abs(input.angle.x) > input_deadzone.get() ||
						   abs(input.angle.y) > input_deadzone.get() ||
						   abs(input.scroll) > 1e-6f;

	if (has_input)
	{
		_noInputTime = 0.0f;
		return;
	}

	_noInputTime += dt;

	if (_noInputTime < delay.get())
		return;

	NodePtr target = ctx.target;
	if (!target)
		return;

	if (require_target_velocity.get())
	{
		Vec3 p = target->getWorldPosition();
		Vec3 v = (p - _lastTargetPos) / dt;
		_lastTargetPos = p;

		if (length(v) < min_target_speed.get())
			return;
	}

	vec3 f = target->getWorldDirection(AXIS_Y);
	f.z = 0.0f;
	Log::error("--- %.5f %.5f %.5f\n", f.x, f.y, f.z);
	if (length2(f) < 1e-6f)
		return;
	f = normalize(f);

	Log::error("%.5f\n", _noInputTime);

	float target_yaw = atan2f(f.x, f.y) * 180.0f / Consts::PI;
	target_yaw += yaw_offset_deg.get();

	state.rig.angle.x = approach_yaw(state.rig.angle.x, target_yaw, dt);
}

float CameraAutoAlignYawRig::approach_yaw(float current, float target, float dt) const
{
	current = normalize_deg(current);
	target = normalize_deg(target);

	float delta = normalize_deg(target - current);

	float a = 1.0f - expf(-speed.get() * dt);
	float step = delta * a;

	float max_step = max_deg_per_sec.get() * dt;
	step = clamp(step, -max_step, max_step);

	return normalize_deg(current + step);
}
