#include "ToggledButton.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(ToggledButton);

void ToggledButton::init()
{
	Button *b = getComponent<Button>(node);
	if (!b)
	{
		Log::error(
			"UI::ToggledButton::init(): Attach this component to Button element! Node: "
			"\"%s\"\n",
			node->getName());
		return;
	}

	button = b->getPtr();
	normal_color = b->bg_color;
	hover_color = b->hover_color;
	press_color = b->press_color;
	focus_color = b->focus_color;
	normal_texture = b->bg_texture_file;
	hover_texture = b->hover_texture;
	press_texture = b->press_texture;
	focus_texture = b->focus_texture;
	button->getEventButtonClicked().connect(*this, [this](Button *b) { setToggled(!toggled); });
}

void ToggledButton::setToggled(bool in_toggled)
{
	if (!button)
		return;

	toggled = in_toggled;

	if (change_color.get() == 1)
	{
		button->bg_color = toggled ? toggled_normal_color : normal_color;
		button->hover_color = toggled ? toggled_hover_color : hover_color;
		button->press_color = toggled ? toggled_press_color : press_color;
		button->focus_color = toggled ? toggled_focus_color : focus_color;
	}
	if (change_texture.get() == 1)
	{
		button->bg_texture_file = toggled ? toggled_normal_texture.get() : normal_texture.get();
		button->hover_texture = toggled ? toggled_hover_texture.get() : hover_texture.get();
		button->press_texture = toggled ? toggled_press_texture.get() : press_texture.get();
		button->focus_texture = toggled ? toggled_focus_texture.get() : focus_texture.get();
	}

	changed_event.run(this);
}
