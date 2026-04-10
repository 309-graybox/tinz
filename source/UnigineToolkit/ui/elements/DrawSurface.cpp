#include "DrawSurface.h"

#include "Canvas.h"

#include <UnigineGame.h>
using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(DrawSurface);

void DrawSurface::init()
{
	if (element_initialized)
		return;

	lock_arrange();
	Element::init();
	unlock_arrange();

	// create
	canvas_w = WidgetCanvas::create(get_gui());
	get_parent_widget()->addChild(canvas_w, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();
}

void DrawSurface::shutdown()
{
	canvas_w.deleteLater();

	Element::shutdown();
}

void DrawSurface::on_enable()
{
	Element::on_enable();
	if (canvas_w)
		canvas_w->setHidden(false);
}

void DrawSurface::on_disable()
{
	if (canvas_w)
		canvas_w->setHidden(true);
	Element::on_disable();
}

void DrawSurface::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int s_x = get_screen_x();
	int s_y = get_screen_y();
	int s_w = max(1, get_screen_width());
	int s_h = max(1, get_screen_height());

	canvas_w->setPosition(s_x, s_y);
	canvas_w->setWidth(s_w);
	canvas_w->setHeight(s_h);
	canvas_w->arrange();
}

void DrawSurface::apply_order_to_widgets()
{
	if (!canvas_w)
		return;
	canvas_w->setOrder(getOrder());
	WidgetPtr parent = canvas_w->getParent();
	parent->removeChild(canvas_w);
	parent->addChild(canvas_w);
}

void DrawSurface::set_gui(const Unigine::GuiPtr &gui)
{
	if (!canvas_w)
		return;
	if (gui != canvas_w->getParentGui())
		gui->addChild(canvas_w);
}
