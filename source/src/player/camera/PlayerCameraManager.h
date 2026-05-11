#pragma once
#include "CameraStageModifier.h"
#include <UnigineComponentSystem.h>

class EIAction;
class EIBinding;

class PlayerCameraManager: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PlayerCameraManager, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(File, action_look_file, "", "", "", "", "filter=.input_action")
	PROP_PARAM(File, action_target_lock_file, "", "", "", "", "filter=.input_action")

	PROP_PARAM(Toggle, search_stages_in_children, false)
	PROP_PARAM(Toggle, rebuild_on_enable, true)

	PROP_PARAM(Node, target_node)
	PROP_PARAM(Node, camera_node)
	PROP_PARAM(Mask, collision_mask, (int)0xffffffff)

	void rebuildPipeline();

	// Gameplay-side request: add `amount` to the shake-trauma accumulator
	// (clamped to [0,1]). The CameraShake stage consumes it. Multiple calls
	// in a frame stack additively. Mirrors the existing target-lock channel —
	// gameplay finds this via getComponent<PlayerCameraManager> on the player
	// node, same as it does for CharacterMovement.
	void addTrauma(float amount);

private:
	void init();
	void update();
	void shutdown();

private:
	void update_camera_state();
	void update_camera_context();

private:
	CameraState _state;
	CameraInput _input;
	CameraContext _ctx;

	Unigine::Vector<CameraStageModifier *> mods;

	EIBinding *_bindingLook = nullptr;
	EIBinding *_bindingTargetLock = nullptr;
	bool _targetLockHeld = false;
};
