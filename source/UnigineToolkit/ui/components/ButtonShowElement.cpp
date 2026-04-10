#include "ButtonShowElement.h"

#include "../../ui/elements/Button.h"
#include "../../ui/elements/Canvas.h"
#include "../../utils/math/EaseInOut.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(ButtonShowElement);

void ButtonShowElement::init()
{
	Button *button = getComponent<Button>(node);
	if (!button)
	{
		Log::error(
			"UI::ButtonShowElement::init(): Attach this component to Button element! Node: "
			"\"%s\"\n",
			node->getName());
		return;
	}

	Element *e = getComponent<Element>(element_node.get());
	if (!e)
	{
		Log::error("UI::ButtonShowElement::init(): \"%s\" node doesn't have Element component!\n",
			element_node.get() ? element_node->getName() : "");
		return;
	}

	// store pointers
	element_ptr = e->getPtr();
	clipmask_ptr = dynamic_ptr_cast<ClipMask>(element_ptr);

	// store variables (state is "show")
	default_world_pos = element_ptr->getWorldPosition(false);
	if (clipmask_ptr)
		default_color = clipmask_ptr->getMaskColor();

	// set initial "show" state and subscribe
	setShow(show_param.get() > 0, true);
	button->getEventButtonClicked().connect(*this, [this]() { setShow(!isShow()); });
}

void ButtonShowElement::update()
{
	if (!element_ptr || progress >= 1)
		return;

	if (duration.get() > 0)
		progress = saturate(progress + Engine::get()->getIFps() / duration.get());
	else
		progress = 1;

	float k = progress;
	switch (move_animation.get())
	{
	case 1:	   // Quad
		k = easeOutQuad(k);
		break;
	case 2:	   // Cubic
		k = easeOutCubic(k);
		break;
	case 3:	   // Quart
		k = easeOutQuart(k);
		break;
	case 4:	   // Quint
		k = easeOutQuint(k);
		break;
	case 5:	   // Expo
		k = easeOutExpo(k);
		break;
	case 6:	   // Sine
		k = easeOutSine(k);
		break;
	case 7:	   // Circ
		k = easeOutCirc(k);
		break;
	case 8:	   // Back
		k = easeOutBack(k);
		break;
	case 9:	   // Elastic
		k = easeOutElastic(k);
		break;
	case 10:	// Bounce
		k = easeOutBounce(k);
		break;
	default:
		break;
	}

	if (move_direction.get() != 0)
	{
		vec4 offset;
		float c_w = element_ptr->getCanvas()->getCanvasWidth();
		float c_h = element_ptr->getCanvas()->getCanvasHeight();
		float s = show ? (1.0f - k) : k;
		switch (move_direction.get())
		{
		case 1:	   // Left
			offset = vec4(c_w, 0, c_w, 0) * s;
			break;
		case 2:	   // Right
			offset = vec4(-c_w, 0, -c_w, 0) * s;
			break;
		case 3:	   // Up
			offset = vec4(0, c_h, 0, c_h) * s;
			break;
		case 4:	   // Down
			offset = vec4(0, -c_h, 0, -c_h) * s;
			break;
		default:
			break;
		}
		element_ptr->setWorldPosition(default_world_pos + offset);
	}

	if (clipmask_ptr)
	{
		vec4 c = default_color;
		c.w = saturate(show ? k : (1.0f - k));
		clipmask_ptr->setMaskColor(c);
	}

	if (progress >= 1 && !show)
		element_ptr->setEnabled(false);
}

void ButtonShowElement::setShow(bool value, bool instant)
{
	if (show != value)
	{
		show = value;
		progress = 1.0f - progress;

		if (show && element_ptr)
			element_ptr->setEnabled(true);
	}

	if (instant)
	{
		progress = 1;
		if (show)
		{
			if (element_ptr)
			{
				element_ptr->setEnabled(true);
				element_ptr->setWorldPosition(default_world_pos);
			}
			if (clipmask_ptr)
				clipmask_ptr->setMaskColor(default_color);
		}
		else
		{
			if (element_ptr)
				element_ptr->setEnabled(false);
		}
	}
}

bool ButtonShowElement::isShow() const
{
	return show;
}
