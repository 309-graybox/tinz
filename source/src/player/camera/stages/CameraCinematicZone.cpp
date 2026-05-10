#include "CameraCinematicZone.h"
#include "components/world/CinematicZone.h"

REGISTER_COMPONENT(CameraCinematicZone)

using namespace Unigine;
using namespace Unigine::Math;

void CameraCinematicZone::addZone(CinematicZone *zone)
{
	_zones.append(zone);
}

void CameraCinematicZone::removeZone(CinematicZone *zone)
{
	_zones.removeOne(zone);
}

void CameraCinematicZone::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	_zones.clear();
	_collisionMask = ctx.collision_mask;
}

void CameraCinematicZone::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	// TODO WTF???
	ctx.collision_mask = _collisionMask;

	if (!ctx.target)
		return;

	CinematicZone *zone = nullptr;
	for (int i = _zones.size() - 1; i != -1; --i)
	{
		if (_zones[i] && _zones[i]->isActive())
		{
			zone = _zones[i];
			break;
		}
	}

	if (!zone)
		return;

	ctx.collision_mask = 0;

	auto zone_node = zone->getNode();

	Vec3 cam_pos = zone_node->getWorldPosition();
	Vec3 target_pos = ctx.targetPosition;
	if (zone->target_node)
		target_pos = zone->target_node->getWorldPosition();

	state.desired_pos = cam_pos;
	state.desired_rot = setTo(cam_pos, target_pos, vec3_up).getRotate();

	state.pos = state.desired_pos;
	state.rot = state.desired_rot;
}
