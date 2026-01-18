#include "CameraRotationLagRig.h"

REGISTER_COMPONENT(CameraRotationLagRig)

using namespace Unigine;
using namespace Unigine::Math;

void CameraRotationLagRig::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	_angle = state.rig.angle;
	_velocity = vec2_zero;

	auto s = (!separate_axis ? vec2(speed) : speed_axis) * 2.0f;
	_omega = vec2(max(s.x, 0.001f), max(s.y, 0.001f));
}

void CameraRotationLagRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	if (only_when_no_input && input.angle == vec2_zero)
	{
		_angle = state.rig.angle;
		state.rig.angle = _angle;
		return;
	}

	auto x = _omega * state.dt;
	auto x2 = x * x;
	auto exp = vec2_one / (vec2_one + x + x2 * 0.48f + x2 * x * 0.235f);

	auto change = _angle - state.rig.angle;
	auto temp = (_velocity + _omega * change) * state.dt;
	_velocity = (_velocity - _omega * temp) * exp;

	_angle = state.rig.angle + (change + temp) * exp;

	state.rig.angle = _angle;
}
