#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class DrawSurface : public ElementWidget
{
public:
	COMPONENT(DrawSurface, ElementWidget);
	PROP_NAME("UI_DrawSurface");

	// smart pointer
	UIPtr<DrawSurface> getPtr() { return UIPtr<DrawSurface>(this); }

	const Unigine::WidgetCanvasPtr &getWidgetCanvas() const { return canvas_w; }

protected:
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void arrange() override;
	void apply_order_to_widgets() override;
	void set_gui(const Unigine::GuiPtr &gui) override;

	Unigine::WidgetCanvasPtr canvas_w;
};
typedef UIPtr<DrawSurface> DrawSurfacePtr;
}	 // namespace UI
