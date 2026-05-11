#pragma once
#include "CameraState.h"
#include "CameraInput.h"
#include "CameraContext.h"
#include <plugins/Ryutp/EnhancedInput/Defines.h>
#include <UnigineComponentSystem.h>

ENUM(CameraStage, Intent, Rig, Constraints, Presentation, Finalize);

class CameraStageModifier: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CameraStageModifier, Unigine::ComponentBase)
	PROP_PARAM(Switch, stage, CameraStage::Rig, Enum<CameraStage>::StringSwitch)
	PROP_PARAM(Int, priority, 0)
	PROP_PARAM(Toggle, used_in_pipeline, true)

	bool isActive() const noexcept { return isEnabled() && used_in_pipeline; }

	virtual void runtimeReset(CameraState &state, const CameraContext &ctx) {}
	virtual void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) = 0;
};
