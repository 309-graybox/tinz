#include "CameraOrbitIntent.h"

REGISTER_COMPONENT(CameraOrbitIntent)

using namespace Unigine;
using namespace Unigine::Math;

void CameraOrbitIntent::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	state.rig.arm_len = default_arm;
	state.rig.pitch_range = pitch_range;
	state.rig.arm_range = arm_range;
}

void CameraOrbitIntent::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	auto before = state.rig.angle;
	auto base = ctx.target ? dvec3(ctx.target->getWorldPosition()) : dvec3_zero;
	state.rig.pivot = base + pivot_offset.get();

	state.rig.angle += input.angle * angle_mul;

	state.rig.angle.y = clamp(state.rig.angle.y, state.rig.pitch_range.x, state.rig.pitch_range.y);
	state.rig.arm_len = clamp(state.rig.arm_len - input.scroll * zoom_speed.get(), state.rig.arm_range.x, state.rig.arm_range.y);
}
