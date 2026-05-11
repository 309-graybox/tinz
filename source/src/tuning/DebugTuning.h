#pragma once
#include "TuningBase.h"

class DebugTuning: public TuningBase<DebugTuning>
{
public:
	COMPONENT_DEFINE(DebugTuning, TuningBase<DebugTuning>)

	PROP_GROUP("Camera")
	PROP_PARAM(Toggle, show_camera_lock_on_direction, false, "", "Draw a red marker at the camera's current lock-on target world position.")

	PROP_GROUP("Movement")
	PROP_PARAM(Toggle, show_movement_base, false, "", "Draw character base vectors (horizontal velocity, move direction, up, gravity, input) and grounded/vertical-speed labels.")
	PROP_PARAM(Toggle, show_movement_rays, false, "", "Draw the downward probe rays used for ground-normal detection.")
	PROP_PARAM(Toggle, show_movement_hit, false, "", "Draw points where ground-probe rays hit a surface.")
	PROP_PARAM(Toggle, show_movement_contact_points, false, "", "Draw collision contact points and their surface normals during movement resolution.")

	PROP_GROUP("Interaction")
	PROP_PARAM(Toggle, show_interaction_trigger, false, "", "Render the player's interaction trigger volume.")
	PROP_PARAM(Toggle, log_interaction, false, "", "Log every Interactable::startInteract call (interactor, target, current state, can-interact).")

	PROP_GROUP("Combat")
	PROP_PARAM(Toggle, show_hitboxes, false, "", "Visualize active hitbox shapes.")
	PROP_PARAM(Toggle, show_hurtboxes, false, "", "Visualize hurtbox shapes.")
	PROP_PARAM(Toggle, show_skulls_direction, false, "", "Draw a line from each Skull enemy to its target, color-coded by behavior (Direct=red, Orbit=blue, Flank=green).")

	PROP_GROUP("Anim")
	PROP_PARAM(Toggle, log_anim_param_on_change, false, "", "Log every change to a character animation parameter as old -> new.")
};
