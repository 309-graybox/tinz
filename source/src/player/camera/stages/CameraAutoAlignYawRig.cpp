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

void CameraAutoAlignYawRig::runtimeReset(CameraState &, const CameraContext &)
{
	_noInputTime = 0.0f;
	_hasLastPos = false;
	_alignActive = false;
	_velLP = Vec3_zero;
}

void CameraAutoAlignYawRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	if (!ctx.target)
		return;

	float dt = state.dt;

	const bool has_input =
		abs(input.angle.x) > input_deadzone.get() ||
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

	Vec3 p = ctx.target->getWorldPosition();
	if (!_hasLastPos)
	{
		_lastPos = p;
		_hasLastPos = true;
		return;
	}

	Vec3 v = (p - _lastPos) / dt;
	_lastPos = p;
	v.z = 0.0f;
	float targetSpeed = length(v);

	if (targetSpeed < 0.1f)
		return;

	Vec3 dir = v / targetSpeed;

	float k = vel_dir_smooth.get();
	float a = 1.0f - expf(-k * dt);

	_velDirSmoothed = normalize(_velDirSmoothed + (dir - _velDirSmoothed) * a);

	float target_yaw = -atan2f(_velDirSmoothed.x, _velDirSmoothed.y) * Consts::RAD2DEG + yaw_offset_deg.get();

	if (!_latched)
	{
		_latchedTargetYaw = target_yaw;
		_latched = true;
	} else
	{
		float dy = normalize_deg(target_yaw - _latchedTargetYaw);
		_latchedTargetYaw += dy * (1.0f - expf(-target_yaw_smooth.get() * dt));
	}

	float delta = normalize_deg(_latchedTargetYaw - state.rig.angle.x);
	float aYaw = 1.0f - expf(-speed.get() * dt);
	float step = clamp(delta * aYaw, -max_deg_per_sec.get() * dt, max_deg_per_sec.get() * dt);
	state.rig.angle.x += step;
}
