#pragma once

#include "../elements/Canvas.h"

#include <UnigineComponentSystem.h>
#include <UnigineObjects.h>

namespace UI {

// Note: place this component to "Object" class node that renders UI
class WorldSpaceUIController : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(WorldSpaceUIController, ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_POST_UPDATE(post_update);	   // after camera moving

	PROP_PARAM(Node, canvas_node, "Canvas", "Canvas that renders UI to this object");
	PROP_PARAM(Node, cursor_element_node, "Cursor Element");
	PROP_PARAM(Float, max_distance, 2.0f, "Max Distance",
		"Maximum distance to interaction with world-space UI");

protected:
	void init();
	void post_update();
	bool get_mouse_coords(Unigine::Math::vec2 &out_pos);

	UI::CanvasPtr canvas;
	UI::ElementPtr cursor_element;
	Unigine::ObjectPtr obj;
	Unigine::ObjectIntersectionTexCoordPtr intersection =
		Unigine::ObjectIntersectionTexCoord::create();
};
}	 // namespace UI
