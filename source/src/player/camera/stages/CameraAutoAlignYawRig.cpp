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
		return;
	}

	_noInputTime += dt;
	if (_noInputTime < delay.get())
		return;

	if (ctx.targetHorizontalSpeed < 0.1f)
		return;

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

	float delta = lerpZero(normalizeAngle(_latchedTargetYaw - state.rig.angle.x), exp(-speed.get() * dt));
	state.rig.angle.x += clamp(delta, -max_deg_per_sec.get() * dt, max_deg_per_sec.get() * dt);
}
