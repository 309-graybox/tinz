#pragma once

#include "SoundManager.h"

#include <UnigineComponentSystem.h>

namespace audio
{

// Place this component on any node in the scene to declare a single sound
// event. On init() it registers itself with the SoundManager under the given
// id, then play2D / play3DAt / playOnNode("my_event") work from anywhere.
//
// Drop one component per event you want to use. To pre-load every event in a
// world, just keep a "Sound Bank" node with N of these attached.
class SoundRegistrator: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(SoundRegistrator, Unigine::ComponentBase)
	COMPONENT_INIT(init, -100000)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_GROUP("Event")
	PROP_PARAM(String, eventId, "", "Event ID", "Identifier used by SoundManager::play*")
	PROP_PARAM(File, sample, "", "Sample", "Audio file", "", "filter=.wav|.oga|.mp3")

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

private:
	void init();
	void shutdown();

	Unigine::String _registered_id;
};

} // namespace audio
