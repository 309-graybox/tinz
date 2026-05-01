#include "MenuButtonStart.h"

#include <UnigineLog.h>
#include <UnigineWorld.h>

REGISTER_COMPONENT(MenuButtonStart);

using namespace Unigine;

void MenuButtonStart::onClick()
{
	const char *path = worldFile.get();
	if (!path || !*path)
	{
		Log::error("MenuButtonStart on '%s': worldFile is empty\n", node ? node->getName() : "<null>");
		return;
	}
	World::loadWorld(path);
}
