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

	// Cursor shown while the menu is open. MOUSE_HANDLE_USER (set by setMouseGrab)
	// has no visible cursor inside the engine window, so we supply a custom one.
	PROP_PARAM(File, cursor_texture, "", "Cursor")
	PROP_PARAM(Vec2, cursor_hotspot, Unigine::Math::vec2_zero, "Cursor Hotspot (px)")

private:
	void init();
	void update();

private:
	void init_main_menu();
	void init_settings_menu();

private:
	// open_with_pad: opened via gamepad -> start with the cursor hidden so the
	// player keeps navigating with the pad until they touch the mouse.
	void toggle_main_menu(bool open_with_pad = false);

	// Hide the pointer while navigating with keyboard/gamepad, show it on mouse
	// move. Runs every frame while a menu is open.
	void update_cursor_visibility();
	void show_cursor();
	void hide_cursor();

private:
	UI::Canvas *_mainMenuCanvas{nullptr};
	UI::Canvas *_settingsMenuCanvas{nullptr};

	Unigine::ImagePtr _cursorImage;
	bool _cursorHidden{false};

	float _prevGameScale{0.0f};
	bool _prevSoundPaused{false};
};
