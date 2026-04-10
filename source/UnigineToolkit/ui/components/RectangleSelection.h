#pragma once

#include "../elements/Canvas.h"
#include "../elements/Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

// Note: attach this component to element!
class RectangleSelection : public Unigine::ComponentBase
{
public:
	COMPONENT(RectangleSelection, ComponentBase);
	PROP_NAME("UIC_RectangleSelection");
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Float, offset, 16.0f);
	PROP_PARAM(Float, rate, 15.0f);

protected:
	void init();
	void update();

	ElementPtr element;
	CanvasPtr canvas;
};
}	 // namespace UI
