#include "CameraAutoAlignYawRig.h"

REGISTER_COMPONENT(CameraAutoAlignYawRig)

using namespace Unigine;
using namespace Unigine::Math;

void CameraAutoAlignYawRig::runtimeReset(CameraState &, const CameraContext &)
{
	_noInputTime = 0.0f;
	_latchedTargetYaw = 0.0f;
	_latched = false;
	_velDirSmoothed = Vec3_forward;
	_currentMaxRate = 0.0f;
}

void CameraAutoAlignYawRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	if (!ctx.target)
		return;

	float dt = state.dt;

	const bool hasInput = abs(input.angle.x) > input_deadzone.get() || abs(input.angle.y) > input_deadzone.get() || abs(input.scroll) > 1e-6f;

	if (hasInput)
	{
		_noInputTime = 0.0f;
		_currentMaxRate = 0.0f;
		return;
	}

	_noInputTime += dt;
	if (_noInputTime < delay.get())
	{
		_currentMaxRate = 0.0f;
		return;
	}

	if (ctx.targetHorizontalSpeed < min_target_velocity)
	{
		if (reset_timer_on_stop)
			_noInputTime = 0.0f;
		_currentMaxRate = 0.0f;
		return;
	}

	_velDirSmoothed = normalize(lerp(_velDirSmoothed, ctx.targetHorizontalVelocity, (1.0f - exp(-vel_dir_smooth * dt))));

	float targetYaw = -atan2(_velDirSmoothed.x, _velDirSmoothed.y) * Consts::RAD2DEG + yaw_offset_deg.get();

	if (!_latched)
	{
		_latchedTargetYaw = targetYaw;
		_latched = true;
	} else
	{
		_latchedTargetYaw += lerpZero(normalizeAngle(targetYaw - _latchedTargetYaw), exp(-target_yaw_smooth.get() * dt));
	}

	const float maxRate = max_deg_per_sec.get();
	if (rate_acceleration_exponential)
	{
		const float k = maxRate > Consts::EPS ? rate_acceleration.get() / maxRate : 0.0f;
		_currentMaxRate += (maxRate - _currentMaxRate) * (1.0f - exp(-k * dt));
	} else
	{
		_currentMaxRate = min(_currentMaxRate + rate_acceleration.get() * dt, maxRate);
	}

	float delta = lerpZero(normalizeAngle(_latchedTargetYaw - state.rig.angle.x), exp(-speed.get() * dt));
	const float allowed = _currentMaxRate * dt;
	state.rig.angle.x += clamp(delta, -allowed, allowed);
}
