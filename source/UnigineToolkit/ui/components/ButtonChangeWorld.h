#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineSounds.h>

namespace UI {

// Note: attach this component to Button element!
class ButtonChangeWorld : public Unigine::ComponentBase
{
public:
	COMPONENT(ButtonChangeWorld, ComponentBase);
	PROP_NAME("UIC_ButtonChangeWorld");
	COMPONENT_INIT(init);

	PROP_PARAM(File, world_file, "", "World", "", "", "filter=.world");

protected:
	void init();
};
}	 // namespace UI
