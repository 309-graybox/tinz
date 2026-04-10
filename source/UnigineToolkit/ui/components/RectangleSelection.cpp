#include "RectangleSelection.h"

#include <UnigineEngine.h>

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(RectangleSelection);

void RectangleSelection::init()
{
	Element *e = getComponent<Element>(node);
	if (!e)
	{
		Log::error(
			"UI::RectangleSelection::init(): Attach this component to Element! Node: \"%s\"\n",
			node->getName());
		return;
	}

	element = e->getPtr();
	canvas = getComponentInParent<Canvas>(node)->getPtr();
}

void RectangleSelection::update()
{
	if (!element || !canvas)
		return;

	Element *focus = canvas->getFocus();
	if (focus)
	{
		vec4 pos = element->getWorldPosition(false);
		vec4 target_pos = focus->getWorldPosition(false);
		target_pos.x -= offset;
		target_pos.y -= offset;
		target_pos.z += offset;
		target_pos.w += offset;
		pos = lerp(pos, target_pos, 1.0f - Math::exp(-rate * Engine::get()->getIFps()));
		element->setWorldPosition(pos);
	}
}
