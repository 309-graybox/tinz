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

	PROP_PARAM(Toggle, debug, true)

	void rebuildPipeline();

private:
	void init();
	void update();
	void shutdown();

private:
	CameraState _state;
	CameraInput _input;
	CameraContext _ctx;

	Unigine::Vector<CameraStageModifier *> mods;

	EIAction *_actionLook = nullptr;
	EIBinding *_bindingLook = nullptr;
	EIAction *_actionTargetLock = nullptr;
	EIBinding *_bindingTargetLock = nullptr;
};
