#pragma once

#include "../elements/Button.h"
#include "../elements/Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

// Note: attach this component to Button element!
class ToggledButton : public Unigine::ComponentBase
{
public:
	COMPONENT(ToggledButton, ComponentBase);
	PROP_NAME("UIC_ToggledButton");
	COMPONENT_INIT(init, -990);

	// clang-format off
	PROP_PARAM(Toggle, change_color, 1, "Change Color");
	PROP_PARAM(Color, toggled_normal_color, Unigine::Math::vec4_white, "Toggled Normal Color", "", "", "change_color=1");
	PROP_PARAM(Color, toggled_hover_color, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "Toggled Hover Color", "", "", "change_color=1");
	PROP_PARAM(Color, toggled_press_color, Unigine::Math::vec4(0.5f, 0.5f, 0.5f, 1.0f), "Toggled Press Color", "", "", "change_color=1");
	PROP_PARAM(Color, toggled_focus_color, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "Toggled Focus Color", "", "", "change_color=1");

	PROP_PARAM(Toggle, change_texture, 0, "Change Texture");
	PROP_PARAM(File, toggled_normal_texture, "", "Toggled Normal Texture", "", "", "change_texture=1");
	PROP_PARAM(File, toggled_hover_texture, "", "Toggled Hover Texture", "", "", "change_texture=1");
	PROP_PARAM(File, toggled_press_texture, "", "Toggled Press Texture", "", "", "change_texture=1");
	PROP_PARAM(File, toggled_focus_texture, "", "Toggled Focus Texture", "", "", "change_texture=1");
	// clang-format on

	// interaction
	void setToggled(bool toggled);
	bool isToggled() const { return toggled; }

	Unigine::Event<ToggledButton *> &getEventChanged() { return changed_event; }

protected:
	void init();

	ButtonPtr button;
	bool toggled = false;

	Unigine::Math::vec4 normal_color;
	Unigine::Math::vec4 hover_color;
	Unigine::Math::vec4 press_color;
	Unigine::Math::vec4 focus_color;

	Unigine::String normal_texture;
	Unigine::String hover_texture;
	Unigine::String press_texture;
	Unigine::String focus_texture;

	// events
	Unigine::EventInvoker<ToggledButton *> changed_event;
};
}	 // namespace UI
