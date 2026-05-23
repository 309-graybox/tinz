#include "MainMenuWorld.h"
#include "MenuButton.h"
#include "MenuDragger.h"
#include "MenuInteractive.h"
#include "audio/SoundManager.h"
#include "utils/Utils.h"

#include <UnigineEngine.h>
#include <UnigineGame.h>
#include <UnigineMaterials.h>
#include <UnigineRender.h>
#include <UnigineWindowManager.h>
#include <UnigineWorld.h>
#include <cstring>

REGISTER_COMPONENT(MainMenuWorld);

using namespace Unigine;
using namespace Math;

void MainMenuWorld::init()
{
	FLOGERR(strcmp(outlineMat, "") != 0, "outline material is not set!\n");
	FLOGERR(_player = Game::getPlayer(), "player not found!\n");

	_outline_material = Materials::findMaterialByPath(outlineMat);
	FLOGERR(_outline_material, "outline material '%s' not found!\n", outlineMat.get());

	_player->addScriptableMaterial(_outline_material);

	setMouseGrab(false);

	_baseline_brightness = Render::getColorCorrectionBrightness();

	ComponentSystem::get()->getComponentsInWorld<MenuInteractive>(_interactives);

	if (auto window = WindowManager::getMainWindow())
	{
		_initial_mouse_pos = window->getClientPosition() + window->getClientSize() / 2;
		Input::setMousePosition(_initial_mouse_pos);
	}
	else
		_initial_mouse_pos = Input::getMousePosition();
	_armed = false;

	const char *music = backgroundMusic.get();
	if (music && *music)
		audio::SoundManager::playMusic(music);
}

void MainMenuWorld::update()
{
	switch (_state)
	{
		case State::Idle: tick_idle(); break;
		case State::PendingClick: tick_pending_click(); break;
		case State::Dragging: tick_dragging(); break;
	}
}

void MainMenuWorld::shutdown()
{
	audio::SoundManager::stopMusic();

	if (_fading)
		Render::setColorCorrectionBrightness(_baseline_brightness);

	Input::clearMouseCursorCustom();
}

void MainMenuWorld::tick_idle()
{
	if (!_armed)
	{
		_input_delay_timer += Game::getIFps();
		if (_input_delay_timer < (float)inputDelay)
			return;
		if (Input::getMousePosition() == _initial_mouse_pos)
			return;
		_armed = true;
	}

	MenuInteractive *hit = raycast_interactive();
	if (hit != _hovered)
	{
		if (_hovered)
			_hovered->setHovered(false);
		if (hit)
			hit->setHovered(true);
		_hovered = hit;
	}

	if (_hovered && Input::isMouseButtonDown(Input::MOUSE_BUTTON_LEFT))
	{
		if (auto *drg = dynamic_cast<MenuDragger *>(_hovered))
			start_drag(drg);
		else if (auto *btn = dynamic_cast<MenuButton *>(_hovered))
			start_press(btn);
	}
}

void MainMenuWorld::tick_pending_click()
{
	if (!_pending_button)
	{
		_state = State::Idle;
		return;
	}

	const float dt = Game::getIFps();
	_pending_timer += dt;

	const float delay = max(_pending_button->getClickDelay(), 1e-4f);
	if (_pending_button->shouldFadeOnClick())
		apply_fade(saturate(_pending_timer / delay));

	float volume = (delay - _pending_timer) / delay;
	Unigine::Sound::setVolume(volume);

	if (_pending_timer >= delay)
	{
		MenuButton *btn = _pending_button;
		_pending_button = nullptr;
		_pending_timer = 0.0f;
		_state = State::Idle;
		btn->onClick();
		btn->release();
	}
}

void MainMenuWorld::tick_dragging()
{
	if (!_active_dragger)
	{
		_state = State::Idle;
		return;
	}

	if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT))
	{
		const ivec2 mp = Input::getMousePosition();
		_active_dragger->updateDrag(mp.y);
	} else
	{
		_active_dragger->endDrag();
		_active_dragger = nullptr;
		_state = State::Idle;
	}

	// Update cursor world point even while dragging, for character look-at.
	raycast_interactive();
}

MenuInteractive *MainMenuWorld::raycast_interactive()
{
	const ivec2 mouse = Input::getMousePosition();
	const dvec3 p0 = _player->getWorldPosition();
	const dvec3 dir = dvec3(_player->getDirectionFromMainWindow(mouse.x, mouse.y));
	const dvec3 p1 = p0 + dir * 100.0;

	WorldIntersectionPtr intersection = WorldIntersection::create();
	ObjectPtr hit = World::getIntersection(p0, p1, intersectionMask, intersection);
	_cursor_point = hit ? intersection->getPoint() : p1;

	return find_interactive_for(hit);
}

MenuInteractive *MainMenuWorld::find_interactive_for(const ObjectPtr &obj) const
{
	if (!obj)
		return nullptr;
	NodePtr cur = obj;
	while (cur)
	{
		for (MenuInteractive *it : _interactives)
		{
			const NodePtr &it_node = it ? it->getNode() : NodePtr();
			if (it_node && it_node->getID() == cur->getID())
				return it;
		}
		cur = cur->getParent();
	}
	return nullptr;
}

void MainMenuWorld::start_press(MenuButton *btn)
{
	if (!btn)
		return;
	btn->press();
	if (_hovered)
	{
		_hovered->setHovered(false, /*play_sound=*/false);
		_hovered = nullptr;
	}
	_pending_button = btn;
	_pending_timer = 0.0f;
	_state = State::PendingClick;

	if (btn->shouldFadeOnClick())
	{
		_baseline_brightness = Render::getColorCorrectionBrightness();
		_fading = true;
	}
}

void MainMenuWorld::start_drag(MenuDragger *drg)
{
	if (!drg)
		return;
	const ivec2 mp = Input::getMousePosition();
	drg->beginDrag(mp.y);
	_active_dragger = drg;
	_state = State::Dragging;
}

void MainMenuWorld::apply_fade(float t01)
{
	const float v = Math::lerp(_baseline_brightness, (float)fadeBrightness, saturate(t01));
	Render::setColorCorrectionBrightness(v);
}
