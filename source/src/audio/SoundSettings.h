#pragma once

#include "SoundManager.h"

#include <UnigineComponentSystem.h>

namespace audio
{

// Reusable bundle of SoundEvent settings (mix / spatial / cone / routing).
// Drop one on a shared "preset" node and reference it from many
// SoundRegistrators via their Settings field, or place it next to a
// SoundRegistrator to override the defaults for a single event.
class SoundSettings: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(SoundSettings, Unigine::ComponentBase)

	PROP_GROUP("Mix")
	PROP_PARAM(Float, gain, 1.0f, "Gain")
	PROP_PARAM(Float, pitchMin, 1.0f, "Pitch Min")
	PROP_PARAM(Float, pitchMax, 1.0f, "Pitch Max", "Random pitch is sampled from [Pitch Min, Pitch Max]")
	PROP_PARAM(Toggle, stream, 0, "Stream", "Stream from disk instead of fully decoding (long files / music)")

	PROP_GROUP("Spatial (3D)")
	PROP_PARAM(Float, minDistance, 1.0f, "Min Distance")
	PROP_PARAM(Float, maxDistance, 100.0f, "Max Distance")
	PROP_PARAM(Float, airAbsorption, 0.0f, "Air Absorption")
	PROP_PARAM(Float, adaptation, 0.0f, "Adaptation")
	PROP_PARAM(Float, roomRolloff, 0.0f, "Room Rolloff")

	PROP_GROUP("Cone (3D)")
	PROP_PARAM(Float, coneInnerAngle, 360.0f, "Inner Angle")
	PROP_PARAM(Float, coneOuterAngle, 360.0f, "Outer Angle")
	PROP_PARAM(Float, coneOuterGain, 0.0f, "Outer Gain")
	PROP_PARAM(Float, coneOuterGainHF, 1.0f, "Outer Gain HF")

	PROP_GROUP("Routing")
	PROP_PARAM(Mask, sourceMask, (int)0xffffffff, "Source Mask")
	PROP_PARAM(Mask, reverbMask, (int)0xffffffff, "Reverb Mask")
	PROP_PARAM(Mask, occlusionMask, 0, "Occlusion Mask")
	PROP_PARAM(Toggle, occlusion, 0, "Occlusion", "Enable occlusion test against Occlusion Mask")

	void applyTo(SoundEvent &e);
};

} // namespace audio
