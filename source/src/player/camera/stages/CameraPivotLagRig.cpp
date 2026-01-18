#include "CameraPivotLagRig.h"

REGISTER_COMPONENT(CameraPivotLagRig)

using namespace Unigine;
using namespace Unigine::Math;

void CameraPivotLagRig::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	_position = state.rig.pivot;
	_velocity = Vec3_zero;

	_omega = (!separate_axis ? Vec3(speed) : Vec3(speed_axis)) * Consts::PI2;
	_expPiece = -(!separate_axis ? Vec3(damping) : Vec3(damping_axis)) * _omega;
}

void CameraPivotLagRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	auto expon = _expPiece * state.dt;
	expon = Vec3(exp(expon.x), exp(expon.y), exp(expon.z));

	auto delta = _position - state.rig.pivot;
	auto temp = (_velocity + delta * _omega) * state.dt;

	_velocity = (_velocity - temp * _omega) * expon;
	_position = state.rig.pivot + (delta + temp) * expon;

	state.rig.pivot = _position;
}
