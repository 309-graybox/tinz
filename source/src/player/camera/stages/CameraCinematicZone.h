#pragma once
#include "player/camera/CameraStageModifier.h"
#include <UnigineWorlds.h>

class CinematicZone;

class CameraCinematicZone: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraCinematicZone, CameraStageModifier)

	void addZone(CinematicZone *zone);
	void removeZone(CinematicZone *zone);

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	Unigine::Vector<CinematicZone *> _zones;
	int _collisionMask = 0;
};
