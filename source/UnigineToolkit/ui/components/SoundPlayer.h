#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineSounds.h>

namespace UI {

// Note: attach this component to ElementFocusable element!
class SoundPlayer : public Unigine::ComponentBase
{
public:
	COMPONENT(SoundPlayer, ComponentBase);
	PROP_NAME("UIC_SoundPlayer");
	COMPONENT_INIT(init, -990);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_PARAM(File, sound_hover_file, "", "Sound Hover", "", "", "filter=.wav|.oga|.mp3");
	PROP_PARAM(File, sound_press_file, "", "Sound Press", "", "", "filter=.wav|.oga|.mp3");
	PROP_PARAM(File, sound_focus_file, "", "Sound Focus", "", "", "filter=.wav|.oga|.mp3");
	PROP_PARAM(Float, volume, 1.0f);

protected:
	void init();
	void shutdown();

	Unigine::AmbientSourcePtr sound_hover;
	Unigine::AmbientSourcePtr sound_press;
	Unigine::AmbientSourcePtr sound_focus;
};
}	 // namespace UI
