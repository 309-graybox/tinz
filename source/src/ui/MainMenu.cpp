#include "MainMenu.h"
#include "UnigineToolkit/ui/elements/Button.h"
#include "UnigineToolkit/ui/elements/Slider.h"
#include <UnigineConsole.h>
#include <UnigineGame.h>
#include <UnigineSounds.h>

#define GET_CANVAS(TO, FROM)             \
	TO = getComponent<UI::Canvas>(FROM); \
	if (!TO)                             \
		return;                          \
	TO->setEnabled(false);

#define GET_BUTTON(FROM, NAME, METHOD)                                 \
	{                                                                  \
		auto btn = dynamic_cast<UI::Button *>(FROM->findChild(#NAME)); \
		if (btn)                                                       \
		{                                                              \
			btn->getEventButtonClicked().connect(*this, METHOD);       \
		}                                                              \
	}

#define GET_SLIDER(FROM, NAME, METHOD)                                    \
	{                                                                     \
		auto slider = dynamic_cast<UI::Slider *>(FROM->findChild(#NAME)); \
		if (slider)                                                       \
		{                                                                 \
			slider->getEventSliderChanged().connect(*this, METHOD);       \
		}                                                                 \
	}

REGISTER_COMPONENT(MainMenu)

using namespace Unigine;

void MainMenu::init()
{
	init_main_menu();
	init_settings_menu();
}

void MainMenu::update()
{
	// TODO tmp solution. Use EI instead
	if (Input::isKeyDown(Input::KEY_ESC) && !Console::isActive())
	{
		if (_settingsMenuCanvas->isEnabled())
		{
			_settingsMenuCanvas->setEnabled(false);
			_mainMenuCanvas->setEnabled(true);
		} else
		{
			toggle_main_menu();
		}
	}

	if (!_mainMenuCanvas->isEnabled() && !_settingsMenuCanvas->isEnabled())
	{
		Input::setMouseHandle(Input::MOUSE_HANDLE_GRAB);
		Input::setMouseGrab(true);
	} else
	{
		Input::setMouseHandle(Input::MOUSE_HANDLE_SOFT);
		Input::setMouseGrab(false);
	}
}

void MainMenu::init_main_menu()
{
	GET_CANVAS(_mainMenuCanvas, main_menu);
	GET_BUTTON(_mainMenuCanvas, ButtonStart, [this] { toggle_main_menu(); });
	GET_BUTTON(_mainMenuCanvas, ButtonRestart, [this] { World::reloadWorld(); });
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
	GET_SLIDER(_settingsMenuCanvas, SliderVolume, [](UI::Slider *s) { Sound::setVolume(s->getValue()); });
	GET_SLIDER(_settingsMenuCanvas, SliderMusicVolume, [this](UI::Slider *s) { Sound::setSourceVolume(sound_music_volume_channel, s->getValue()); });
}

void MainMenu::toggle_main_menu()
{
	if (_mainMenuCanvas->isEnabled())
	{
		Game::setScale(_prevGameScale);

		_mainMenuCanvas->setEnabled(false);
		Input::setMouseHandle(Input::MOUSE_HANDLE_GRAB);
		Input::setMouseGrab(true);

		_mainMenuCanvas->setEnabled(false);
	} else
	{
		_prevGameScale = Game::getScale();
		Game::setScale(0.0f);

		_mainMenuCanvas->setEnabled(true);
		Input::setMouseHandle(Input::MOUSE_HANDLE_SOFT);
		Input::setMouseGrab(false);

		_mainMenuCanvas->setEnabled(true);
	}
}
