#include "SliderStateColors.h"

REGISTER_COMPONENT(SliderStateColors)

using namespace Unigine;
using namespace Unigine::Math;

void SliderStateColors::update()
{
	// Resolve the sibling slider lazily — same-node component init order isn't
	// guaranteed, so we wait until the first update.
	if (!_resolved)
	{
		_slider = getComponent<UI::Slider>(node);
		_resolved = true;
	}
	if (!_slider)
		return;

	const Unigine::WidgetSpriteShaderPtr &bg = _slider->getWidgetSpriteShaderBackground();
	const Unigine::WidgetSpriteShaderPtr &fg = _slider->getWidgetSpriteShaderForeground();
	if (!bg || !fg)
		return; // slider widgets not created yet

	const UI::ElementFocusable::State state = _slider->getState();
	const float t = _slider->getStateTransitionTime();

	if (!_seeded)
	{
		_bg_now = _bg_prev = _slider->getBackgroundColor();
		_fg_now = _fg_prev = _slider->getForegroundColor();
		_last_state = state;
		_seeded = true;
	}

	// On a state change, start the blend from the currently displayed color so it
	// eases from where it is instead of snapping/flashing through the base color.
	if (state != _last_state)
	{
		_bg_prev = _bg_now;
		_fg_prev = _fg_now;
		_last_state = state;
	}

	if (animate_bg.get())
	{
		const vec4 target = pick_color(
			state, _slider->getBackgroundColor(), bg_hover, bg_press, bg_focus, bg_inactive);
		_bg_now = lerp(_bg_prev, target, t);
		bg->setColor(_bg_now);
	}
	if (animate_fg.get())
	{
		const vec4 target = pick_color(
			state, _slider->getForegroundColor(), fg_hover, fg_press, fg_focus, fg_inactive);
		_fg_now = lerp(_fg_prev, target, t);
		fg->setColor(_fg_now);
	}
}

vec4 SliderStateColors::pick_color(UI::ElementFocusable::State state, const vec4 &normal,
	const vec4 &hover, const vec4 &press, const vec4 &focus, const vec4 &inactive) const
{
	using State = UI::ElementFocusable::State;
	switch (state)
	{
		case State::Hover:
			return hover;
		case State::Press:
			return press;
		case State::Focus:
			return focus;
		case State::Inactive:
			return inactive;
		default:
			return normal;
	}
}
