#pragma once

#include "SoundManager.h"

#include <UnigineComponentSystem.h>

namespace audio
{

// Place this component on any node in the scene to declare a single sound
// event. On init() it registers itself with the SoundManager under the given
// id, then play2D / play3DAt / playOnNode("my_event") work from anywhere.
//
// Per-event settings (mix/spatial/cone/routing) live on a SoundSettings
// component. Resolution order: the node referenced by Settings → this node →
// SoundEvent defaults — so a shared preset node can drive many registrators
// while still allowing per-event overrides.
class SoundRegistrator: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(SoundRegistrator, Unigine::ComponentBase)
	COMPONENT_INIT(init, -100000)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_GROUP("Event")
	PROP_PARAM(String, eventId, "", "Event ID", "Identifier used by SoundManager::play*")
	PROP_PARAM(File, sample, "", "Sample", "Audio file", "", "filter=.wav|.oga|.mp3")
	PROP_PARAM(String, loopEventId, "", "Loop Event ID", "Optional SoundManager event id looped after Sample intro")
	PROP_PARAM(Node, settings, "Settings", "SoundSettings node; falls back to self, then defaults")

private:
	void init();
	void shutdown();

	Unigine::String _registered_id;
};

} // namespace audio
