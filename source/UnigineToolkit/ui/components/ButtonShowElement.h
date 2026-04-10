#pragma once

#include "../elements/ClipMask.h"

#include <UnigineComponentSystem.h>

namespace UI {

// Note: attach this component to Button element!
class ButtonShowElement : public Unigine::ComponentBase
{
public:
	COMPONENT(ButtonShowElement, ComponentBase);
	PROP_NAME("UIC_ButtonShowElement");
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Toggle, show_param, 0, "Show");
	PROP_PARAM(Node, element_node, "Element");

	PROP_PARAM(Float, duration, 0.2f);
	PROP_PARAM(Switch, move_direction, 0, "None,Left,Right,Up,Down");
	PROP_PARAM(Switch, move_animation, 0,
		"Lerp,Quad,Cubic,Quart,Quint,Expo,Sine,Circ,Back,Elastic,Bounce");

	void setShow(bool value, bool instant = false);
	bool isShow() const;

protected:
	void init();
	void update();

	bool show = false;
	float progress = 1;

	ElementPtr element_ptr;
	ClipMaskPtr clipmask_ptr;
	Unigine::Math::vec4 default_world_pos;
	Unigine::Math::vec4 default_color;
};
}	 // namespace UI
