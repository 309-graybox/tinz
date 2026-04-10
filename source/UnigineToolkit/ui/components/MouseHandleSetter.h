#pragma once

#include <UnigineComponentSystem.h>

namespace UI {

// Note: you can place this component to any node
class MouseHandleSetter : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(MouseHandleSetter, ComponentBase);
	COMPONENT_INIT(init, -2000 /*before Canvas::init_canvas()*/);

	PROP_PARAM(Switch, mode, 2, "GRAB,SOFT,USER", "Mode",
		"GRAB: use this if you control the camera with the mouse (PlayerActor, PlayerSpectator),\n"
		"      and the 2D UI is used primarily for information.\n"
		"SOFT: use this in a mixed mode, when the camera can be rotated and you can interact with\n"
		"      the 2D UI at the same time.\n"
		"USER: use this when you only control the 2D UI.");
	PROP_PARAM(Toggle, grab_at_start, 1, "Grab At Start", "", "", "mode=0");

protected:
	void init();
};
}	 // namespace UI
