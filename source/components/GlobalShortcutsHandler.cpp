#include <UnigineComponentSystem.h>
#include <UnigineInput.h>

using namespace Unigine;

class GlobalShortcutsHandler: public ComponentBase
{
public:
	COMPONENT_DEFINE(GlobalShortcutsHandler, ComponentBase)
	COMPONENT_UPDATE(update)

private:
	void update()
	{
		if (!Input::isModifierEnabled(Input::MODIFIER_LEFT_CTRL))
			return;

		if (Input::isKeyDown(Input::KEY_O))
			World::reloadWorld();
		else if (Input::isKeyDown(Input::KEY_P))
			Engine::get()->quit();
	}
};

REGISTER_COMPONENT(GlobalShortcutsHandler)
