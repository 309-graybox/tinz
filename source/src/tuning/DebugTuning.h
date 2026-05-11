#pragma once
#include "TuningBase.h"

class DebugTuning: public TuningBase<DebugTuning>
{
public:
	COMPONENT_DEFINE(DebugTuning, TuningBase<DebugTuning>)

	PROP_GROUP("Camera")
	PROP_PARAM(Toggle, show_camera_lock_on_direction, false)

	PROP_GROUP("Movement")
	PROP_PARAM(Toggle, show_movement_base, false)
	PROP_PARAM(Toggle, show_movement_rays, false)
	PROP_PARAM(Toggle, show_movement_hit, false)
	PROP_PARAM(Toggle, show_movement_contact_points, false)

	PROP_GROUP("Interaction")
	PROP_PARAM(Toggle, show_interaction_trigger, false)
	PROP_PARAM(Toggle, log_interaction, false)

	PROP_GROUP("Combat")
	PROP_PARAM(Toggle, show_hitboxes, false)
	PROP_PARAM(Toggle, show_hurtboxes, false)
	PROP_PARAM(Toggle, show_skulls_direction, false)
};
