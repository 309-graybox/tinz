#include "ButtonChangeWorld.h"

#include "../../ui/elements/Button.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(ButtonChangeWorld);

void ButtonChangeWorld::init()
{
	Button *button = getComponent<Button>(node);
	if (!button)
	{
		Log::error(
			"UI::ButtonChangeWorld::init(): Attach this component to Button element! Node: "
			"\"%s\"\n",
			node->getName());
		return;
	}

	button->getEventButtonClicked().connect(*this, [this]() { World::loadWorld(world_file); });
}
