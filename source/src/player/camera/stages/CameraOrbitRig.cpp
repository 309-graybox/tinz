#include "CameraOrbitRig.h"

REGISTER_COMPONENT(CameraOrbitRig)

using namespace Unigine;
using namespace Unigine::Math;

void CameraOrbitRig::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	build(state, ctx);
}

void CameraOrbitRig::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	build(state, ctx);
}

void CameraOrbitRig::build(CameraState &state, const CameraContext &ctx) const
{
	auto qyaw = quat(vec3_up, state.rig.angle.x);
	auto q_orbit = quat(qyaw * vec3_right, state.rig.angle.y) * qyaw;
	auto pos = state.rig.pivot - q_orbit * dvec3_forward * state.rig.arm_len + (q_orbit * state.rig.local_offset);
	auto cam_rot = q_orbit * quat(vec3_right, 90.0f);

	state.desired_pos = Vec3(pos);
	state.desired_rot = cam_rot;

	state.pos = state.desired_pos;
	state.rot = state.desired_rot;
}
