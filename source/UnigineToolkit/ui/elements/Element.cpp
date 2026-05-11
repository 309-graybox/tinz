#include "Element.h"

#include "../../utils/math/EaseInOut.h"
#include "Canvas.h"

#include <UnigineViewport.h>

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Element);

void Element::init()
{
	applyNodeHierarchyChanges(false);

	// note:
	// applyNodeHierarchyChanges uses addChild() inside
	// addChild() uses analogue of applyPropertyChanges() inside
	// so, we don't need to use applyPropertyChanges() here

	element_initialized = true;
}

void Element::shutdown()
{
	// run callbacks, notify subscribers
	destroy_event.run(this);

	// detach from parent
	if (parent)
		parent->removeChild(this);

	// notify control block
	if (ptr_control_block)
	{
		if (ptr_control_block->counter > 0)
			ptr_control_block->is_deleted = true;
		else
			delete ptr_control_block;
	}
}

void Element::applyPropertyChanges()
{
	arrange_hierarchy();
	update_order_hierarchy();
}

void Element::applyPropertyChangesRecursively()
{
	applyPropertyChanges();

	// update children
	for (auto child : children)
		if (child->isElementInitialized())
			child->applyPropertyChangesRecursively();
}

void Element::applyNodeHierarchyChanges(bool save_screen_position)
{
	NodePtr parent_node = node->getParent() ? node->getParent() : node->getPossessor();
	if (parent_node)
	{
		vec4 pos;
		bool has_canvas_before = (canvas != nullptr);
		if (has_canvas_before && save_screen_position)
			pos = getWorldPosition();

		if (parent)
			parent->removeChild(this);

		parent = getComponentInParent<Element>(parent_node);
		if (parent)
		{
			// first initialize parent element
			if (!parent->isElementInitialized())
				parent->initializeElement();

			// then attach to it
			parent->addChild(this);
		}

		if (has_canvas_before && save_screen_position)
			setWorldPosition(pos);
	} else if (parent)
		parent->removeChild(this);
}

void Element::initializeElement()
{
	if (!element_initialized)
		init();
}

void Element::setParent(Element *in_parent)
{
	if (in_parent)
		in_parent->addChild(this);
	else if (parent)
		parent->removeChild(this);
}

void Element::addChild(Element *element)
{
	UNIGINE_ASSERT(element && this != element);

	if (children.findIndex(element) != -1)
		return; // added already

	// detach from previous parent
	if (element->parent)
		element->parent->removeChild(element);

	// attach to this
	children.append(element);
	element->parent = this;
	element->set_canvas_hierarchy(canvas);
	element->update_enabled_hierarchy();
	element->arrange_hierarchy();
	element->update_order_hierarchy();

	// notify canvas
	if (canvas)
		canvas->on_add_element(element);
}

void Element::removeChild(Element *element)
{
	if (children.findIndex(element) == -1)
		return; // removed already

	children.removeOne(element);
	element->parent = nullptr;

	// notify canvas
	if (canvas)
		canvas->on_remove_element(element);
}

void Element::setChildIndex(Element *element, int index)
{
	int prev_index = getChildIndex(element);
	if (prev_index == -1)
		return;

	index = clamp(index, 0, children.size() - 1);
	if (prev_index == index)
		return;

	if (index < prev_index)
	{
		for (int i = prev_index; i > index; --i)
			children[i] = children[i - 1];
	} else
	{
		for (int i = prev_index; i < index; i++)
			children[i] = children[i + 1];
	}
	children[index] = element;
}

int Element::getChildIndex(Element *element) const
{
	for (int i = 0; i < children.size(); i++)
	{
		if (children[i] == element)
			return i;
	}
	return -1;
}

Element *Element::findChild(const char *name, bool recursive) const
{
	for (int i = 0; i < children.size(); i++)
	{
		Element *child = children[i];
		if (strcmp(child->getNode()->getName(), name) == 0)
			return child;

		if (recursive)
		{
			if (Element *find_in_children = child->findChild(name, recursive))
				return find_in_children;
		}
	}

	return nullptr;
}

void Element::setOrderOffset(int offset)
{
	order_offset = offset;
	update_order_hierarchy();
}

void Element::setWorldPosition(float left, float top, float right, float bottom)
{
	if (!canvas)
		return;

	float cw = canvas->getCanvasWidth();
	float ch = canvas->getCanvasHeight();
	vec4 parent_pos = parent ? vec4(parent->min_n.x * cw, parent->min_n.y * ch,
								   parent->max_n.x * cw, parent->max_n.y * ch)
							 : vec4(0, 0, cw, ch);

	vec4 a = anchor;
	float x = (left - Math::lerp(parent_pos.x, parent_pos.z, a.x));
	float y = (top - Math::lerp(parent_pos.y, parent_pos.w, a.y));
	float w = (right - left);
	float h = (bottom - top);

	// apply pivot
	x += pivot.get().x * w;
	y += pivot.get().y * h;

	// apply position, width and height (in canvas coords)
	vec4 p = vec4(x, y, w, h);

	// what if anchor is non fixed:
	bool stretched_x = a.x != a.z;
	bool stretched_y = a.y != a.w;
	if (stretched_x)
	{
		p.x = (left - Math::lerp(parent_pos.x, parent_pos.z, a.x));
		p.z = (Math::lerp(parent_pos.x, parent_pos.z, a.z) - right);
	}
	if (stretched_y)
	{
		p.y = (top - Math::lerp(parent_pos.y, parent_pos.w, a.y));
		p.w = (Math::lerp(parent_pos.y, parent_pos.w, a.w) - bottom);
	}

	pos = p;

	arrange_hierarchy();
}

void Element::setWorldPosition(const vec2 &left_top_pos, const vec2 &right_bottom_pos)
{
	setWorldPosition(left_top_pos.x, left_top_pos.y, right_bottom_pos.x, right_bottom_pos.y);
}

void Element::setWorldPosition(const vec4 &pos)
{
	setWorldPosition(pos.x, pos.y, pos.z, pos.w);
}

vec4 Element::getWorldPosition(bool round_position) const
{
	float w = canvas->getCanvasWidth();
	float h = canvas->getCanvasHeight();
	if (round_position)
		return vec4(Math::round(min_n.x * w), Math::round(min_n.y * h), Math::round(max_n.x * w),
			Math::round(max_n.y * h));
	else
		return vec4(min_n.x * w, min_n.y * h, max_n.x * w, max_n.y * h);
}

void Element::setPositionLeftTop(float x, float y)
{
	vec4 p = pos;
	if (isFixedWidth())
		p.x = x + pivot.get().x * p.z;
	else
	{
		float diff = x - p.x;
		p.x = x;
		p.z -= diff;
	}
	if (isFixedHeight())
		p.y = y + pivot.get().y * p.w;
	else
	{
		float diff = y - p.y;
		p.y = y;
		p.w -= diff;
	}
	pos = p;

	arrange_hierarchy();
}

void Element::setPositionLeftTop(const Unigine::Math::vec2 &pos)
{
	setPositionLeftTop(pos.x, pos.y);
}

Unigine::Math::vec2 Element::getPositionLeftTop() const
{
	vec4 p = pos;

	if (isFixedWidth())
		p.x = p.x - pivot.get().x * p.z;

	if (isFixedHeight())
		p.y = p.y - pivot.get().y * p.w;

	return vec2(p.x, p.y);
}

void Element::setSize(float width, float height)
{
	vec4 p = pos;
	if (isFixedWidth())
		p.z = width;
	else if (canvas)
	{
		float cw = canvas->getCanvasWidth();
		vec4 a = anchor;
		vec2 parent_points =
			parent ? vec2(parent->min_n.x * cw, parent->max_n.x * cw) : vec2(0, cw);
		float parent_width = (a.z - a.x) * (parent_points.y - parent_points.x);
		float left_offset = pivot.get().x * (parent_width - width);
		float right_offset = width - parent_width + left_offset;
		p.x = left_offset;
		p.z = -right_offset;
	}
	if (isFixedHeight())
		p.w = height;
	else if (canvas)
	{
		float ch = canvas->getCanvasHeight();
		vec4 a = anchor;
		vec2 parent_points =
			parent ? vec2(parent->min_n.y * ch, parent->max_n.y * ch) : vec2(0, ch);
		float parent_height = (a.w - a.y) * (parent_points.y - parent_points.x);
		float top_offset = pivot.get().y * (parent_height - height);
		float bottom_offset = height - parent_height + top_offset;
		p.y = top_offset;
		p.w = -bottom_offset;
	}
	pos = p;
	arrange_hierarchy();
}

void Element::setSize(float size)
{
	setSize(size, size);
}

void Element::setSize(const Unigine::Math::vec2 &size)
{
	setSize(size.x, size.y);
}

Unigine::Math::vec2 Element::getSize() const
{
	float width = 0;
	float height = 0;

	if (isFixedWidth())
		width = pos.get().z;
	else if (canvas)
		width = (max_n.x - min_n.x) * canvas->getCanvasWidth();

	if (isFixedHeight())
		height = pos.get().w;
	else if (canvas)
		height = (max_n.y - min_n.y) * canvas->getCanvasHeight();

	return vec2(width, height);
}

void Element::setPivot(float x, float y)
{
	pivot = vec2(x, y);
	arrange_hierarchy();
}

void Element::setPivotX(float x)
{
	pivot = vec2(x, pivot.get().y);
	arrange_hierarchy();
}

void Element::setPivotY(float y)
{
	pivot = vec2(pivot.get().x, y);
	arrange_hierarchy();
}

void Element::setPivot(const Unigine::Math::vec2 &in_pivot)
{
	pivot = in_pivot;
	arrange_hierarchy();
}

void Element::setAnchor(float x_min, float y_min, float x_max, float y_max)
{
	anchor = Unigine::Math::vec4(x_min, y_min, x_max, y_max);
	arrange_hierarchy();
}

void Element::setAnchor(const Unigine::Math::vec4 &in_anchor)
{
	anchor = in_anchor;
	arrange_hierarchy();
}

bool Element::isFixedWidth() const
{
	vec4 a = anchor;
	return Math::compare(a.x, a.z);
}

bool Element::isFixedHeight() const
{
	vec4 a = anchor;
	return Math::compare(a.y, a.w);
}

void Element::setAnchorLeftTop(bool change_pivot)
{
	anchor = Unigine::Math::vec4(0, 0, 0, 0);
	if (change_pivot)
		pivot = vec2(0, 0);
	arrange_hierarchy();
}

void Element::setAnchorLeftMiddle(bool change_pivot)
{
	anchor = Unigine::Math::vec4(0.0f, 0.5f, 0.0f, 0.5f);
	if (change_pivot)
		pivot = vec2(0.0f, 0.5f);
	arrange_hierarchy();
}

void Element::setAnchorLeftBottom(bool change_pivot)
{
	anchor = Unigine::Math::vec4(0, 1, 0, 1);
	if (change_pivot)
		pivot = vec2(0, 1);
	arrange_hierarchy();
}

void Element::setAnchorCenterTop(bool change_pivot)
{
	anchor = Unigine::Math::vec4(0.5f, 0.0f, 0.5f, 0.0f);
	if (change_pivot)
		pivot = vec2(0.5f, 0.0f);
	arrange_hierarchy();
}

void Element::setAnchorCenterMiddle(bool change_pivot)
{
	anchor = Unigine::Math::vec4(0.5f, 0.5f, 0.5f, 0.5f);
	if (change_pivot)
		pivot = vec2(0.5f, 0.5f);
	arrange_hierarchy();
}

void Element::setAnchorCenterBottom(bool change_pivot)
{
	anchor = Unigine::Math::vec4(0.5f, 1.0f, 0.5f, 1.0f);
	if (change_pivot)
		pivot = vec2(0.5f, 1.0f);
	arrange_hierarchy();
}

void Element::setAnchorRightTop(bool change_pivot)
{
	anchor = Unigine::Math::vec4(1, 0, 1, 0);
	if (change_pivot)
		pivot = vec2(1, 0);
	arrange_hierarchy();
}

void Element::setAnchorRightMiddle(bool change_pivot)
{
	anchor = Unigine::Math::vec4(1.0f, 0.5f, 1.0f, 0.5f);
	if (change_pivot)
		pivot = vec2(1.0f, 0.5f);
	arrange_hierarchy();
}

void Element::setAnchorRightBottom(bool change_pivot)
{
	anchor = Unigine::Math::vec4(1, 1, 1, 1);
	if (change_pivot)
		pivot = vec2(1, 1);
	arrange_hierarchy();
}

void Element::setPosition(const Unigine::Math::vec2 &position)
{
	setPosition(position.x, position.y);
}

void Element::setPosition(float in_x, float in_y)
{
	vec4 p = pos;
	pos = vec4(in_x, in_y, p.z, p.w);
	arrange_hierarchy();
}

void Element::setPositionX(float in_x)
{
	vec4 p = pos;
	pos = vec4(in_x, p.y, p.z, p.w);
	arrange_hierarchy();
}

void Element::setPositionY(float in_y)
{
	vec4 p = pos;
	pos = vec4(p.x, in_y, p.z, p.w);
	arrange_hierarchy();
}

vec2 Element::getPosition() const
{
	vec4 p = pos;
	return vec2(p.x, p.y);
}

void Element::setWidth(float width)
{
	vec4 p = pos;
	pos = vec4(p.x, p.y, width, p.w);
	arrange_hierarchy();
}

void Element::setHeight(float height)
{
	vec4 p = pos;
	pos = vec4(p.x, p.y, p.z, height);
	arrange_hierarchy();
}

void Element::setAnchorExpand()
{
	anchor = Unigine::Math::vec4(0, 0, 1.f, 1.f);
	arrange_hierarchy();
}

void Element::setAnchorStretchHTop(bool change_pivot_y)
{
	anchor = Unigine::Math::vec4(0, 0, 1, 0);
	if (change_pivot_y)
		pivot = vec2(pivot.get().x, 0);
	arrange_hierarchy();
}

void Element::setAnchorStretchHMiddle(bool change_pivot_y)
{
	anchor = Unigine::Math::vec4(0.0f, 0.5f, 1.0f, 0.5f);
	if (change_pivot_y)
		pivot = vec2(pivot.get().x, 0.5f);
	arrange_hierarchy();
}

void Element::setAnchorStretchHBottom(bool change_pivot_y)
{
	anchor = Unigine::Math::vec4(0, 1, 1, 1);
	if (change_pivot_y)
		pivot = vec2(pivot.get().x, 1);
	arrange_hierarchy();
}

void Element::setAnchorStretchVLeft(bool change_pivot_x)
{
	anchor = Unigine::Math::vec4(0, 0, 0, 1);
	if (change_pivot_x)
		pivot = vec2(0, pivot.get().y);
	arrange_hierarchy();
}

void Element::setAnchorStretchVCenter(bool change_pivot_x)
{
	anchor = Unigine::Math::vec4(0.5f, 0.0f, 0.5f, 1.0f);
	if (change_pivot_x)
		pivot = vec2(0.5f, pivot.get().y);
	arrange_hierarchy();
}

void Element::setAnchorStretchVRight(bool change_pivot_x)
{
	anchor = Unigine::Math::vec4(1, 0, 1, 1);
	if (change_pivot_x)
		pivot = vec2(1, pivot.get().y);
	arrange_hierarchy();
}

void Element::setLeftOffset(float offset)
{
	vec4 p = pos;
	pos = vec4(offset, p.y, p.z, p.w);
	arrange_hierarchy();
}

void Element::setRightOffset(float offset)
{
	vec4 p = pos;
	pos = vec4(p.x, p.y, offset, p.w);
	arrange_hierarchy();
}

void Element::setTopOffset(float offset)
{
	vec4 p = pos;
	pos = vec4(p.x, offset, p.z, p.w);
	arrange_hierarchy();
}

void Element::setBottomOffset(float offset)
{
	vec4 p = pos;
	pos = vec4(p.x, p.y, p.z, offset);
	arrange_hierarchy();
}

bool Element::isHover(int x, int y) const
{
	if (!isEnabled() || !is_in_render_area(x, y))
		return false;

	// check point selection
	int w = canvas->getScreenWidth();
	int h = canvas->getScreenHeight();
	ivec4 b = ivec4(ftoi(min_n.x * w), ftoi(min_n.y * h), ftoi(max_n.x * w), ftoi(max_n.y * h));
	if (x >= b.x && x <= b.z && y >= b.y && y <= b.w)
		return true;

	return false;
}

bool Element::isHover(int x0, int y0, int x1, int y1) const
{
	if (!isEnabled() || !is_in_render_area(x0, y0, x1, y1))
		return false;

	// check rectangle intersecion
	int w = canvas->getScreenWidth();
	int h = canvas->getScreenHeight();
	ivec4 b = ivec4(ftoi(min_n.x * w), ftoi(min_n.y * h), ftoi(max_n.x * w), ftoi(max_n.y * h));
	if (x0 < b.z && x1 > b.x && y0 < b.w && y1 > b.y)
		return true;

	return false;
}

void Element::on_enable()
{
	if (canvas)
		canvas->needToRearrange();
}

void Element::on_disable()
{
}

void Element::update_hierarchy(float ifps)
{
	if (!node->isEnabled())
		return;

	// update itself
	if (isEnabled()) // if enabled property
		update(ifps);

	// update children
	for (int i = 0; i < children.size(); ++i)
		children[i]->update_hierarchy(ifps);
}

void Element::update_enabled_hierarchy()
{
	if (parent && parent->isEnabled() != node->isEnabled())
		node->setEnabled(parent->isEnabled());

	for (int i = 0; i < children.size(); ++i)
		children[i]->update_enabled_hierarchy();
}

void Element::set_canvas_hierarchy(UI::ICanvas *in_canvas)
{
	if (canvas == in_canvas)
		return;

	canvas = in_canvas;
	set_gui(canvas ? canvas->getGui() : Gui::getCurrent());

	for (auto &child : children)
		child->set_canvas_hierarchy(canvas);
}

void Element::update_order_hierarchy()
{
	for (auto &child : children)
		child->update_order_hierarchy();

	apply_order_to_widgets();
}

void Element::arrange_hierarchy()
{
	if (!node->isEnabled())
		return;

	update_bound();
	arrange(); // call derived logic

	// update children
	for (auto child : children)
		child->arrange_hierarchy();

	arrange_event.run(this);
}

void Element::update_bound()
{
	// apply parent min/max and achor
	vec4 a = anchor;
	min_n = vec2_zero;
	max_n = vec2_one;
	if (parent)
	{
		float parent_width_n = parent->max_n.x - parent->min_n.x;
		float parent_height_n = parent->max_n.y - parent->min_n.y;

		min_n.x = parent->min_n.x + a.x * parent_width_n;
		min_n.y = parent->min_n.y + a.y * parent_height_n;
		max_n.x = parent->min_n.x + a.z * parent_width_n;
		max_n.y = parent->min_n.y + a.w * parent_height_n;
	} else
	{
		min_n.x = a.x;
		min_n.y = a.y;
		max_n.x = a.z;
		max_n.y = a.w;
	}

	// get normalized position (offsets) of the element
	vec4 pos_n;
	vec4 p = pos;
	if (canvas)
	{
		pos_n.x = p.x / canvas->getCanvasWidth();
		pos_n.y = p.y / canvas->getCanvasHeight();
		pos_n.z = p.z / canvas->getCanvasWidth();
		pos_n.w = p.w / canvas->getCanvasHeight();
	} else
	{
		GuiPtr gui = Gui::getCurrent();
		pos_n.x = p.x / gui->getWidth();
		pos_n.y = p.y / gui->getHeight();
		pos_n.z = p.z / gui->getWidth();
		pos_n.w = p.w / gui->getHeight();
	}

	// apply pos
	if (!Math::compare(a.x, a.z)) // stretched
	{
		min_n.x += pos_n.x; // offset
		max_n.x -= pos_n.z; // offset
	} else
	{
		min_n.x += pos_n.x - pos_n.z * pivot.get().x; // pos x
		max_n.x = min_n.x + pos_n.z;				  // width
	}

	if (!Math::compare(a.y, a.w)) // stretched
	{
		min_n.y += pos_n.y; // offset
		max_n.y -= pos_n.w; // offset
	} else
	{
		min_n.y += pos_n.y - pos_n.w * pivot.get().y; // pos y
		max_n.y = min_n.y + pos_n.w;				  // height
	}
}

GuiPtr Element::get_gui() const
{
	if (canvas)
		return canvas->getGui();

	// default gui
	return Gui::getCurrent();
}

WidgetVBoxPtr Element::get_parent_widget() const
{
	return get_gui()->getVBox();
}

bool Element::is_in_render_area(int x, int y) const
{
	if (!canvas)
		return false;

	// check point inside render area
	int rx = canvas->getRenderPositionX();
	int ry = canvas->getRenderPositionY();
	int rz = rx + canvas->getRenderWidth();
	int rw = ry + canvas->getRenderHeight();
	if (x < rx || x > rz || y < ry || y > rw)
		return false;

	return true;
}

bool Element::is_in_render_area(int x0, int y0, int x1, int y1) const
{
	if (!canvas)
		return false;

	// check rect intersect render area
	int rx = canvas->getRenderPositionX();
	int ry = canvas->getRenderPositionY();
	int rz = rx + canvas->getRenderWidth();
	int rw = ry + canvas->getRenderHeight();
	if (!(x0 < rz && x1 > rx && y0 < rw && y1 > ry))
		return false;

	return true;
}

bool Element::is_rect_rect_intersection(int x0, int y0, int x1, int y1, const vec2 &r1_pos,
	const mat2 &r1_rot, const vec2 &r1_size) const
{
	// returns true if two rectangles are intersected (one of them is oriented)
	// x0, y0, x1, y1 - left-top and right-bottom corners of the first rectangle (not oriented)
	// r1_pos - center of the second oriented rectangle
	// r1_rot - rotation matrix of the second oriented rectangle
	// r1_size - width and height of the second rectangle

	vec2 h0 = vec2(0.5f * (x1 - x0), 0.5f * (y1 - y0));
	vec2 h1 = r1_size * 0.5f;

	const mat2 &c = r1_rot;
	mat2 abs_c(Math::abs(c[0]), Math::abs(c[1]), Math::abs(c[2]), Math::abs(c[3]));

	// SAT intersection relative to rect0
	vec2 dp = r1_pos - vec2(x0 + h0.x, y0 + h0.y);
	vec2 face0 = Math::abs(dp) - h0 - abs_c * h1;
	if (face0.x > 0.0f || face0.y > 0.0f)
		return false;

	// SAT intersection relative to rect1
	mat2 rot1t = transpose(r1_rot);
	vec2 d1 = rot1t * dp;
	mat2 abs_ct = transpose(abs_c);
	vec2 face1 = Math::abs(d1) - abs_ct * h0 - h1;
	if (face1.x > 0.0f || face1.y > 0.0f)
		return false;

	return true;
};

int Element::get_screen_x() const
{
	if (canvas)
		return ftoi(Math::round(min_n.x * canvas->getScreenWidth()));
	return ftoi(Math::round(min_n.x * Gui::getCurrent()->getWidth()));
}

int Element::get_screen_y() const
{
	if (canvas)
		return ftoi(Math::round(min_n.y * canvas->getScreenHeight()));
	return ftoi(Math::round(min_n.y * Gui::getCurrent()->getHeight()));
}

int Element::get_screen_width() const
{
	if (isFixedWidth())
	{
		if (canvas)
			return canvas->convertCanvasToScreen(getWidth());
		return ftoi(getWidth());
	} else
	{
		if (canvas)
			return ftoi(Math::round((max_n.x - min_n.x) * canvas->getScreenWidth()));
		return ftoi(Math::round((max_n.x - min_n.x) * Gui::getCurrent()->getWidth()));
	}
}

int Element::get_screen_height() const
{
	if (isFixedHeight())
	{
		if (canvas)
			return canvas->convertCanvasToScreen(getHeight());
		return ftoi(getHeight());
	} else
	{
		if (canvas)
			return ftoi(Math::round((max_n.y - min_n.y) * canvas->getScreenHeight()));
		return ftoi(Math::round((max_n.y - min_n.y) * Gui::getCurrent()->getHeight()));
	}
}

void ElementWidget::lock_arrange()
{
	arrange_behavior = ArrangeBehavior::DEFERRED;
}

bool ElementWidget::is_arrange_locked()
{
	if (arrange_behavior != ArrangeBehavior::INSTANT)
	{
		arrange_behavior = ArrangeBehavior::NEED_TO_ARRANGE;
		return true;
	}
	return false;
}

bool ElementWidget::unlock_arrange()
{
	auto state = arrange_behavior;
	arrange_behavior = ArrangeBehavior::INSTANT;
	if (state == ArrangeBehavior::NEED_TO_ARRANGE)
		return true;
	return false;
}

void ElementFocusable::setActive(bool enabled)
{
	active = enabled;
}

void ElementFocusable::setFocusable(bool value)
{
	focusable = value ? 1 : 0;

	if (!value && focus)
		setFocus(false);
}

void ElementFocusable::setFocus(bool value)
{
	value = isFocusable() ? value : false;
	if (focus == value)
		return;

	focus = value;
	arrange();
	canvas->on_focus_changed(this);
}

void ElementFocusable::set_state(bool active, bool hovering, bool clicking)
{
	State next_state = State::Normal;
	bool instant = false;

	if (!active)
		next_state = State::Inactive;
	else if (clicking)
	{
		if (hovering)
		{
			next_state = State::Press;
			instant = true;
		} else if (focus)
			next_state = State::Focus;
	} else if (focus)
		next_state = State::Focus;
	else if (hovering)
		next_state = State::Hover;

	set_state(next_state, instant);
}

void ElementFocusable::set_state(State in_state, bool instant)
{
	if (state == in_state)
		return;

	prev_color = get_color();
	prev_scale = get_scale();

	state = in_state;
	transition_percent = instant ? 1.0f : 0.0f;

	change_state_event.run(this);
}

void ElementFocusable::update_state(float ifps)
{
	float duration = animation_duration.get();
	if (duration <= 0)
		transition_percent = 1;
	else
		transition_percent = saturate(transition_percent + ifps / duration);
}

Unigine::Math::vec4 ElementFocusable::get_color(const Unigine::Math::vec4 &normal_color) const
{
	vec4 target_color = normal_color;
	switch (state)
	{
		case State::Hover:
			target_color = hover_color;
			break;
		case State::Press:
			target_color = press_color;
			break;
		case State::Focus:
			target_color = focus_color;
			break;
		case State::Inactive:
			target_color = inactive_color;
			break;
		default:
			break;
	}

	return lerp(prev_color, target_color, transition_percent);
}

const char *ElementFocusable::get_texture(const char *normal_texture) const
{
	const char *target_texture = nullptr;
	switch (state)
	{
		case State::Hover:
			target_texture = hover_texture.get();
			break;
		case State::Press:
			target_texture = press_texture.get();
			break;
		case State::Focus:
			target_texture = focus_texture.get();
			break;
		case State::Inactive:
			target_texture = inactive_texture.get();
			break;
		default:
			break;
	}
	if (!target_texture || target_texture[0] == '\0')
		target_texture = normal_texture;

	if (!target_texture || target_texture[0] == '\0')
		target_texture = "core/textures/common/white.texture";

	return target_texture;
}

float ElementFocusable::get_scale(float normal_scale) const
{
	float target_scale = normal_scale;
	switch (state)
	{
		case State::Hover:
			target_scale = hover_scale;
			break;
		case State::Press:
			target_scale = press_scale;
			break;
		case State::Focus:
			target_scale = focus_scale;
			break;
		case State::Inactive:
			target_scale = inactive_scale;
			break;
		default:
			break;
	}

	float k = transition_percent;
	switch (scale_animation.get())
	{
		case 1: // Quad
			k = easeOutQuad(transition_percent);
			break;
		case 2: // Cubic
			k = easeOutCubic(transition_percent);
			break;
		case 3: // Quart
			k = easeOutQuart(transition_percent);
			break;
		case 4: // Quint
			k = easeOutQuint(transition_percent);
			break;
		case 5: // Expo
			k = easeOutExpo(transition_percent);
			break;
		case 6: // Sine
			k = easeOutSine(transition_percent);
			break;
		case 7: // Circ
			k = easeOutCirc(transition_percent);
			break;
		case 8: // Back
			k = easeOutBack(transition_percent);
			break;
		case 9: // Elastic
			k = easeOutElastic(transition_percent);
			break;
		case 10: // Bounce
			k = easeOutBounce(transition_percent);
			break;
		default:
			break;
	}

	return Math::lerp(prev_scale, target_scale, k);
}

void MaterialDefaultVariablesSetter::setMaterial(const MaterialPtr &in_mat)
{
	if (mat == in_mat)
		return;

	mat = in_mat;
	if (mat)
	{
		p_texture_size = mat->findParameter("texture_size");
		p_sprite_pos = mat->findParameter("sprite_pos");
		p_sprite_size = mat->findParameter("sprite_size");
		p_screen_size = mat->findParameter("screen_size");
		p_mouse_pos = mat->findParameter("mouse_pos");
		has_update_expression = true;
		has_post_expression = true;

		connection.disconnect();
		if (EngineWindowViewportPtr window = WindowManager::getMainWindow())
		{
			window->getViewport()->getEventEndPostMaterials().connect(connection, [this]() {
				if (has_post_expression)
					has_post_expression =
						mat->runExpression("RENDER_CALLBACK_END_POST_MATERIALS", 0, 0);
				if (!has_post_expression)
					connection.disconnect();
			});
		}
	} else
	{
		p_texture_size = -1;
		p_sprite_pos = -1;
		p_sprite_size = -1;
		p_screen_size = -1;
		p_mouse_pos = -1;
		has_update_expression = false;
		has_post_expression = false;
	}
}

void MaterialDefaultVariablesSetter::setTextureSize(const vec3 &texture_size)
{
	if (p_texture_size != -1)
		mat->setParameterInt2("texture_size", ivec2(ftoi(texture_size.x), ftoi(texture_size.y)));
}

void MaterialDefaultVariablesSetter::setSpritePosAndSize(
	const Unigine::WidgetPtr &widget, const Unigine::Math::ivec2 &screen_size)
{
	if (p_sprite_pos != -1)
	{
		mat->setParameterInt2("sprite_pos", ivec2(widget->getPositionX(), widget->getPositionY()));
	}

	if (p_sprite_size != -1)
	{
		mat->setParameterInt2("sprite_size", ivec2(widget->getWidth(), widget->getHeight()));
	}

	if (p_screen_size != -1)
	{
		mat->setParameterInt2("screen_size", screen_size);
	}
}

void MaterialDefaultVariablesSetter::setMousePosition(
	const Unigine::WidgetPtr &widget, const Unigine::Math::ivec2 &mouse_pos)
{
	if (p_mouse_pos != -1)
	{
		float x = itof(mouse_pos.x - widget->getScreenPositionX()) / widget->getWidth();
		float y = itof(mouse_pos.y - widget->getScreenPositionY()) / widget->getHeight();
		mat->setParameterFloat2("mouse_pos", vec2(x, y));
	}
}

void MaterialDefaultVariablesSetter::runExpressionUpdate(const Unigine::WidgetPtr &widget)
{
	if (has_update_expression)
		has_update_expression =
			mat->runExpression("update", widget->getWidth(), widget->getHeight());
}
