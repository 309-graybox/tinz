#include "MainMenu.h"
#include "utils/Utils.h"
#include "utils/ToolkitUtils.h"
#include "UnigineToolkit/ui/elements/Button.h"
#include "UnigineToolkit/ui/elements/Slider.h"
#include "audio/SoundManager.h"
#include <UnigineConsole.h>
#include <UnigineGame.h>
#include <UnigineImage.h>
#include <UnigineLog.h>
#include <UnigineSounds.h>
#include <UnigineWindowManager.h>

REGISTER_COMPONENT(MainMenu)

using namespace Unigine;

void MainMenu::init()
{
	init_main_menu();
	init_settings_menu();

	if (const char *path = cursor_texture.get(); path && *path)
	{
		_cursorImage = Image::create();
		if (!_cursorImage->load(path))
		{
			Log::warning("MainMenu: failed to load cursor image '%s'\n", path);
			_cursorImage.clear();
		}
	}
}

void MainMenu::update()
{
	// TODO tmp solution. Use EI instead
	bool pad_back = false;
	if (Input::getNumGamePads() > 0)
	{
		if (auto gamepad = Input::getGamePad(0))
			pad_back = gamepad->isButtonDown(Input::GAMEPAD_BUTTON_START)
					   || gamepad->isButtonDown(Input::GAMEPAD_BUTTON_B);
	}
	bool back_pressed = Input::isKeyDown(Input::KEY_ESC) || pad_back;

	if (back_pressed && !Console::isActive())
	{
		if (_settingsMenuCanvas->isEnabled())
		{
			_settingsMenuCanvas->setEnabled(false);
			_mainMenuCanvas->setEnabled(true);
		} else
		{
			toggle_main_menu(pad_back);
		}
	}

	if (_mainMenuCanvas->isEnabled() || _settingsMenuCanvas->isEnabled())
		update_cursor_visibility();
}

void MainMenu::init_main_menu()
{
	GET_CANVAS(_mainMenuCanvas, main_menu);
	GET_BUTTON(_mainMenuCanvas, ButtonStart, [this] { toggle_main_menu(); });
	GET_BUTTON(_mainMenuCanvas, ButtonRestart, [] { World::reloadWorld(); });
	GET_BUTTON(_mainMenuCanvas, ButtonSettings, [this] {
		_mainMenuCanvas->setEnabled(false);
		_settingsMenuCanvas->setEnabled(true);
	});
	GET_BUTTON(_mainMenuCanvas, ButtonQuit, [] { Engine::get()->quit(); });
}

void MainMenu::init_settings_menu()
{
	GET_CANVAS(_settingsMenuCanvas, settings_menu);
	GET_SLIDER(_settingsMenuCanvas, SliderBrightness, [](UI::Slider *s) { Render::setColorCorrectionBrightness(s->getValue()); });
	GET_SLIDER(_settingsMenuCanvas, SliderContrast, [](UI::Slider *s) { Render::setColorCorrectionContrast(s->getValue()); });
	GET_SLIDER(_settingsMenuCanvas, SliderGamma, [](UI::Slider *s) { Render::setColorCorrectionGamma(s->getValue()); });
	// GET_SLIDER(_settingsMenuCanvas, SliderWhite, [](UI::Slider *s) { Render::setColorCorrectionWhite({s->getValue(), s->getValue(), s->getValue(), 1.0f}); });
	GET_SLIDER(_settingsMenuCanvas, SliderVolume, [](UI::Slider *s) { audio::SoundManager::setMasterVolume(s->getValue()); });
}

void MainMenu::toggle_main_menu(bool open_with_pad)
{
	if (_mainMenuCanvas->isEnabled())
	{
		Game::setScale(_prevGameScale);
		audio::SoundManager::setPaused(_prevSoundPaused);

		_mainMenuCanvas->setEnabled(false);

		setMouseGrab(true);
		Input::clearMouseCursorCustom();

		_mainMenuCanvas->setEnabled(false);
	} else
	{
		_prevGameScale = Game::getScale();
		_prevSoundPaused = audio::SoundManager::isPaused();
		Game::setScale(0.0f);
		audio::SoundManager::setPaused(true);

		_mainMenuCanvas->setEnabled(true);

		setMouseGrab(false);
		// USER handle has no cursor inside the engine window; supply a custom one.
		// Opened from the gamepad -> start hidden so the pad keeps driving the menu.
		if (open_with_pad)
			hide_cursor();
		else
			show_cursor();
		if (auto window = WindowManager::getMainWindow())
			Input::setMousePosition(window->getClientPosition() + window->getClientSize() / 2);

		_mainMenuCanvas->setEnabled(true);
	}
}

void MainMenu::update_cursor_visibility()
{
	const auto delta = Input::getMouseDeltaPosition();
	// small threshold so the cursor-recenter caused by grabbing (see hide_cursor)
	// doesn't immediately read as movement and flicker the cursor back on
	const bool mouse_moved = abs(delta.x) > 2 || abs(delta.y) > 2;
	const bool mouse_active = mouse_moved || Input::getMouseWheel() != 0
							  || Input::isMouseButtonDown(Input::MOUSE_BUTTON_LEFT);

	bool nav_input = Input::isKeyDown(Input::KEY_UP) || Input::isKeyDown(Input::KEY_DOWN)
					 || Input::isKeyDown(Input::KEY_LEFT) || Input::isKeyDown(Input::KEY_RIGHT)
					 || Input::isKeyDown(Input::KEY_ENTER);

	if (Input::getNumGamePads() > 0)
	{
		if (auto gamepad = Input::getGamePad(0))
			nav_input = nav_input || gamepad->isButtonDown(Input::GAMEPAD_BUTTON_DPAD_UP)
						|| gamepad->isButtonDown(Input::GAMEPAD_BUTTON_DPAD_DOWN)
						|| gamepad->isButtonDown(Input::GAMEPAD_BUTTON_DPAD_LEFT)
						|| gamepad->isButtonDown(Input::GAMEPAD_BUTTON_DPAD_RIGHT)
						|| gamepad->isButtonDown(Input::GAMEPAD_BUTTON_A)
						|| gamepad->getAxesLeft().length() > 0.5f;
	}

	// Mouse movement wins: any motion brings the pointer back. Otherwise hide it
	// the moment the player drives the menu with the keyboard or gamepad.
	if (mouse_active)
	{
		if (_cursorHidden)
			show_cursor();
	}
	else if (nav_input && !_cursorHidden)
		hide_cursor();
}

void MainMenu::show_cursor()
{
	Input::setMouseGrab(false);
	Input::setMouseCursorHide(false);
	Input::setMouseHandle(Input::MOUSE_HANDLE_USER);
	if (_cursorImage)
	{
		const Unigine::Math::vec2 hs = cursor_hotspot.get();
		Input::setMouseCursorCustom(_cursorImage, static_cast<int>(hs.x), static_cast<int>(hs.y));
	}

	// Drop the keyboard/gamepad focus so the stale "active" highlight doesn't
	// linger on a button once the player is back on the mouse (hover drives it).
	auto clear_focus = [](UI::Canvas *c) {
		if (c && c->isEnabled())
			if (UI::ElementFocusable *f = c->getFocus())
				f->setFocus(false);
	};
	clear_focus(_mainMenuCanvas);
	clear_focus(_settingsMenuCanvas);

	_cursorHidden = false;
}

void MainMenu::hide_cursor()
{
	// setMouseCursorHide alone doesn't remove a custom cursor — only grabbing the
	// mouse actually hides it. Safe to grab in the pause menu: camera look bails
	// on dt<=0 (Game is paused -> getIFps()==0), and we grab via raw Input only,
	// so ControlsApp stays disabled and the view can't rotate.
	Input::clearMouseCursorCustom();
	Input::setMouseHandle(Input::MOUSE_HANDLE_GRAB);
	Input::setMouseGrab(true);
	Input::setMouseCursorHide(true);
	_cursorHidden = true;
}
