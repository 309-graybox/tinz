#include "CameraAutoAlignPitchRig.h"

REGISTER_COMPONENT(CameraAutoAlignPitchRig)

using namespace Unigine;
using namespace Unigine::Math;

void CameraAutoAlignPitchRig::runtimeReset(CameraState &, const CameraContext &)
{
	_noInputTime = 0.0f;
	_latchedTargetPitch = 0.0f;
	_latched = false;
	_velDirSmoothed = Vec3_forward;
}

void CameraAutoAlignPitchRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	if (!ctx.target)
		return;

	const float dt = state.dt;

	const bool hasInput =
		abs(input.angle.x) > input_deadzone.get() ||
		abs(input.angle.y) > input_deadzone.get() ||
		abs(input.scroll)  > 1e-6f;

	if (hasInput)
	{
		_noInputTime = 0.0f;
		_latched = false;
		return;
	}

	_noInputTime += dt;
	if (_noInputTime < delay.get())
		return;

	const Vec3 v = ctx.targetVelocity;

	const float v_len = length(v);
	if (v_len < 0.1f)
		return;

	_velDirSmoothed = normalize(lerp(_velDirSmoothed, v, (1.0f - exp(-vel_dir_smooth.get() * dt))));

	const float h = sqrt(_velDirSmoothed.x * _velDirSmoothed.x + _velDirSmoothed.y * _velDirSmoothed.y);
	if (h < 1e-4f)
		return;

	float targetPitch = atan2(_velDirSmoothed.z, h) * Consts::RAD2DEG + pitch_offset_deg.get();
	targetPitch = clamp(targetPitch, min_pitch_deg.get(), max_pitch_deg.get());

	if (!_latched)
	{
		_latchedTargetPitch = targetPitch;
		_latched = true;
	}
	else
	{
		_latchedTargetPitch += lerpZero(normalizeAngle(targetPitch - _latchedTargetPitch),
										exp(-target_pitch_smooth.get() * dt));
	}

	float delta = lerpZero(normalizeAngle(_latchedTargetPitch - state.rig.angle.y), exp(-speed.get() * dt));
	delta = clamp(delta, -max_deg_per_sec.get() * dt, max_deg_per_sec.get() * dt);

	state.rig.angle.y += delta;
	state.rig.angle.y = clamp(state.rig.angle.y, min_pitch_deg.get(), max_pitch_deg.get());
}
