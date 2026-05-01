#include "MenuButtonExit.h"

#include <UnigineEngine.h>

REGISTER_COMPONENT(MenuButtonExit);

void MenuButtonExit::onClick()
{
	Unigine::Engine::get()->quit();
}
