#pragma once
#include <UnigineComponentSystem.h>
#include <UniginePhysics.h>
#include <UnigineWorlds.h>

// Drop on a node that owns or references a WorldTrigger. When the player walks
// into the trigger, the checkpoint records its world position (or that of an
// optional spawn anchor) into GameState so the next death respawns there.
class Checkpoint: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Checkpoint, Unigine::ComponentBase)
	COMPONENT_INIT(init)

	PROP_PARAM(Node, trigger, "Trigger", "WorldTrigger that detects player entry — required")
	PROP_PARAM(Node, spawn_anchor, "Spawn Anchor", "Optional — spawn position is read from this node's world position. If empty, uses the trigger's position")
	PROP_PARAM(Toggle, once, true, "Once", "Latch on first activation; further enters are ignored")
	PROP_PARAM(String, soundOnActivate, "", "Activate Sound", "SoundManager event id (or sample path) played on activation; empty = silent")

private:
	void init();
	void onEnter(const Unigine::NodePtr &n);

	Unigine::WorldTriggerPtr _trigger;
	bool _consumed = false;
};
