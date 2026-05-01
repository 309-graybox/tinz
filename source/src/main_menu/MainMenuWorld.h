#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineInput.h>
#include <UnigineMaterial.h>
#include <UnigineMathLib.h>
#include <UniginePlayers.h>
#include <UnigineVector.h>


class MenuInteractive;
class MenuButton;
class MenuDragger;


class MainMenuWorld : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(MainMenuWorld, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_PARAM(File, outlineMat)
	PROP_PARAM(String, backgroundMusic)
	PROP_PARAM(Mask, intersectionMask, ~0)
	PROP_PARAM(Float, fadeBrightness, -1.0f, "Fade Brightness", "Target color-correction brightness during click fade (-1 = black)")

	const Unigine::Math::dvec3 &getCursorWorldPoint() const noexcept { return _cursor_point; }
	MenuInteractive *getHovered() const noexcept { return _hovered; }

private:
	enum class State
	{
		Idle,
		PendingClick,
		Dragging,
	};

	void init();
	void update();
	void shutdown();

	MenuInteractive *raycast_interactive();
	MenuInteractive *find_interactive_for(const Unigine::ObjectPtr &obj) const;

	void tick_idle();
	void tick_pending_click();
	void tick_dragging();

	void start_press(MenuButton *btn);
	void start_drag(MenuDragger *drg);
	void apply_fade(float t01);

	Unigine::PlayerPtr _player;
	Unigine::MaterialPtr _outline_material;
	Unigine::Vector<MenuInteractive *> _interactives;

	State _state = State::Idle;
	MenuInteractive *_hovered = nullptr;

	// Pending click
	MenuButton *_pending_button = nullptr;
	float _pending_timer = 0.0f;
	bool _fading = false;
	float _baseline_brightness = 1.0f;

	// Dragging
	MenuDragger *_active_dragger = nullptr;

	Unigine::Math::dvec3 _cursor_point = Unigine::Math::dvec3_zero;

	Unigine::Input::MOUSE_HANDLE _mouse_handle = Unigine::Input::MOUSE_HANDLE_USER;
};

