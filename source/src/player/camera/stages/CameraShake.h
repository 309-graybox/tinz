#pragma once
#include "player/camera/CameraStageModifier.h"

// Trauma-driven camera shake. Lives at the Presentation stage so it runs
// AFTER positioning (Rig) and AFTER collision constraints — adds a small
// positional + roll oscillation on top of the final framing.
//
// Set Stage = Presentation in the editor (base default is Rig).
//
// Reads CameraState::trauma (0..1) — gameplay code requests it via
// PlayerCameraManager::addTrauma. Visible shake scales as trauma² (so 0.5
// trauma is already mild, 1.0 is screen-rocking) and decays at
// recoveryPerSecond.
class CameraShake final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraShake, CameraStageModifier)

	PROP_PARAM(Float, maxOffset, 0.15f, "Max Offset", "Maximum positional shake (m) at trauma=1.0")
	PROP_PARAM(Float, maxRotDeg, 2.0f, "Max Rotation", "Maximum angular roll (degrees) at trauma=1.0")
	PROP_PARAM(Float, recoveryPerSecond, 1.6f, "Recovery", "Trauma decay rate. 1.0 = full→empty in 1 second; bigger = snappier")
	PROP_PARAM(Float, frequency, 25.0f, "Frequency", "Oscillation rate (Hz). 20-30 looks 'kick'-y, 8-12 looks 'rumble'-y")

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	float _phase = 0.0f;
};
