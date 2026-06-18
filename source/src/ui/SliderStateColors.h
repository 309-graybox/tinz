#pragma once

#include "UnigineToolkit/ui/elements/Slider.h"
#include <UnigineComponentSystem.h>

// Per-state background/foreground tint for a UI::Slider.
//
// Lives as a separate game-side component (attach it to the same node as the
// Slider) rather than as Slider params, because the toolkit's editor plugin owns
// UI_Slider.prop and regenerates it — any params added to the Slider itself get
// stripped in the editor. This component's property is owned by the game, so it
// stays editable in the editor and isn't touched by the toolkit plugin.
//
// Reads the slider's state + transition each frame and paints its background /
// foreground sprites, blending from the current color so transitions are smooth.
class SliderStateColors : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(SliderStateColors, Unigine::ComponentBase)
	COMPONENT_UPDATE(update)

	// Normal state uses the slider's own Background/Foreground Color.
	PROP_GROUP("Background")
	PROP_PARAM(Toggle, animate_bg, 1, "Animate Background")
	PROP_PARAM(Color, bg_hover, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "BG Hover", "", "Background", "animate_bg=1")
	PROP_PARAM(Color, bg_press, Unigine::Math::vec4(0.5f, 0.5f, 0.5f, 1.0f), "BG Press", "", "Background", "animate_bg=1")
	PROP_PARAM(Color, bg_focus, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "BG Focus", "", "Background", "animate_bg=1")
	PROP_PARAM(Color, bg_inactive, Unigine::Math::vec4(0.8f, 0.8f, 0.8f, 0.8f), "BG Inactive", "", "Background", "animate_bg=1")

	PROP_GROUP("Foreground")
	PROP_PARAM(Toggle, animate_fg, 0, "Animate Foreground")
	PROP_PARAM(Color, fg_hover, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "FG Hover", "", "Foreground", "animate_fg=1")
	PROP_PARAM(Color, fg_press, Unigine::Math::vec4(0.5f, 0.5f, 0.5f, 1.0f), "FG Press", "", "Foreground", "animate_fg=1")
	PROP_PARAM(Color, fg_focus, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "FG Focus", "", "Foreground", "animate_fg=1")
	PROP_PARAM(Color, fg_inactive, Unigine::Math::vec4(0.8f, 0.8f, 0.8f, 0.8f), "FG Inactive", "", "Foreground", "animate_fg=1")

private:
	void update();

	Unigine::Math::vec4 pick_color(UI::ElementFocusable::State state,
		const Unigine::Math::vec4 &normal, const Unigine::Math::vec4 &hover,
		const Unigine::Math::vec4 &press, const Unigine::Math::vec4 &focus,
		const Unigine::Math::vec4 &inactive) const;

	UI::Slider *_slider{nullptr};
	bool _resolved{false};

	bool _seeded{false};
	UI::ElementFocusable::State _last_state{UI::ElementFocusable::State::Normal};
	Unigine::Math::vec4 _bg_now, _bg_prev, _fg_now, _fg_prev;
};
