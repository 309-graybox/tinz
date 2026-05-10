#pragma once
#include <UnigineComponentSystem.h>
#include <UnigineWorlds.h>
#include <plugins/Ryutp/EnhancedInput/Defines.h>

class PlayerCameraManager;

class CinematicZone: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CinematicZone, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Toggle, active, true)
	PROP_PARAM(Node, bound_trigger)
	PROP_PARAM(Node, target_node)

	bool isActive() const noexcept { return active; }

	Unigine::Math::Vec3 getTargetPosition(const Unigine::Math::Vec3 &p) noexcept { return target_node ? target_node->getWorldPosition() : p; }

private:
	void init();
	void update();

private:
	void onEnter(const Unigine::NodePtr &n);
	void onLeave(const Unigine::NodePtr &n);

private:
	Unigine::WorldTriggerPtr _boundTrigger;
};
