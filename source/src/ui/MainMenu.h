#pragma once
#include "UnigineToolkit/ui/elements/Canvas.h"

class MainMenu: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(MainMenu, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Node, main_menu)
	PROP_PARAM(Node, settings_menu)

	PROP_PARAM(Mask, sound_music_volume_channel, 0x1)

private:
	void init();
	void update();

private:
	void init_main_menu();
	void init_settings_menu();

private:
	void toggle_main_menu();

private:
	UI::Canvas *_mainMenuCanvas{nullptr};
	UI::Canvas *_settingsMenuCanvas{nullptr};

	float _prevGameScale{0.0f};
	bool _prevSoundPaused{false};
};
