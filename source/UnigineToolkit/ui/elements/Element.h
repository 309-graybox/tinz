#pragma once

#include <UnigineComponentSystem.h>

namespace UI {

//////////////////////////////////////////////////////////////
// Define Helpers
//////////////////////////////////////////////////////////////

#define GET_GUID(FILE_NAME)                                                                        \
	Unigine::String::format("guid://%s", Unigine::FileSystem::getGUID(FILE_NAME).makeString().get())

//////////////////////////////////////////////////////////////
// Smart Pointers
//////////////////////////////////////////////////////////////

struct UIPtrControlBlock
{
	volatile int counter = 0;
	volatile bool is_deleted = false;
};

template<typename T>
class UIPtr
{
public:
	UIPtr()
		: ptr(nullptr)
		, ptr_control_block(nullptr)
	{}

	explicit UIPtr(std::nullptr_t)
		: ptr(nullptr)
		, ptr_control_block(nullptr)
	{}

	explicit UIPtr(T *ptr)
		: ptr(ptr)
	{
		if (ptr)
		{
			ptr_control_block = ptr->ptr_control_block;
			increment_counter();
		}
		else
			ptr_control_block = nullptr;
	}

	template<typename OtherType,
		class = typename std::enable_if<std::is_convertible<OtherType *, T *>::value, void>::type>
	explicit UIPtr(OtherType *other_ptr)
	{
		if (other_ptr)
		{
			ptr = static_cast<OtherType *>(other_ptr);
			ptr_control_block = ptr->ptr_control_block;
			increment_counter();
		}
		else
			ptr_control_block = nullptr;
	}

	UIPtr(const UIPtr &other)
		: ptr(other.ptr)
		, ptr_control_block(other.ptr_control_block)
	{
		increment_counter();
	}

	UIPtr(UIPtr &&other) noexcept
	{
		ptr = other.ptr;
		ptr_control_block = other.ptr_control_block;
		is_owner = other.is_owner;
		other.ptr = nullptr;
		other.ptr_control_block = nullptr;
		other.is_owner = false;
	}

	template<typename OtherType,
		class = typename std::enable_if<std::is_convertible<OtherType *, T *>::value, void>::type>
	UIPtr(const UIPtr<OtherType> &other)
	{
		ptr = static_cast<OtherType *>(other.ptr);
		ptr_control_block = other.ptr_control_block;
		increment_counter();
	}

	template<typename OtherType,
		class = typename std::enable_if<std::is_convertible<OtherType *, T *>::value, void>::type>
	UIPtr(UIPtr<OtherType> &&other) noexcept
	{
		ptr = static_cast<OtherType *>(other.ptr);
		ptr_control_block = other.ptr_control_block;
		is_owner = other.is_owner;
		other.ptr = nullptr;
		other.ptr_control_block = nullptr;
		other.is_owner = false;
	}

	UIPtr &operator=(const UIPtr &other)
	{
		if (this != &other)
		{
			decrement_counter();
			ptr = other.ptr;
			ptr_control_block = other.ptr_control_block;
			increment_counter();
		}
		return *this;
	}

	UIPtr &operator=(UIPtr &&other) noexcept
	{
		if (this != &other)
		{
			decrement_counter();
			ptr = other.ptr;
			ptr_control_block = other.ptr_control_block;
			is_owner = other.is_owner;
			other.ptr = nullptr;
			other.ptr_control_block = nullptr;
			other.is_owner = false;
		}
		return *this;
	}

	template<typename OtherType,
		class = typename std::enable_if<std::is_convertible<OtherType *, T *>::value, void>::type>
	UIPtr &operator=(const UIPtr<OtherType> &other)
	{
		decrement_counter();
		ptr = static_cast<OtherType *>(other.ptr);
		ptr_control_block = other.ptr_control_block;
		increment_counter();
		return *this;
	}

	template<typename OtherType,
		class = typename std::enable_if<std::is_convertible<OtherType *, T *>::value, void>::type>
	UIPtr &operator=(UIPtr<OtherType> &&other) noexcept
	{
		decrement_counter();
		ptr = static_cast<OtherType *>(other.ptr);
		ptr_control_block = other.ptr_control_block;
		is_owner = other.is_owner;
		other.ptr = nullptr;
		other.ptr_control_block = nullptr;
		other.is_owner = false;
		return *this;
	}

	~UIPtr() { decrement_counter(); }

	T *get() const { return (ptr_control_block && !ptr_control_block->is_deleted) ? ptr : nullptr; }

	T *operator->() const
	{
		return (ptr_control_block && !ptr_control_block->is_deleted) ? ptr : nullptr;
	}

	T &operator*() const
	{
		assert(!isNull());
		return *get();
	}

	explicit operator bool() const { return !isNull(); }

	bool isNull() const { return ptr_control_block == nullptr || ptr_control_block->is_deleted; }

	void clear()
	{
		decrement_counter();
		ptr = nullptr;
		ptr_control_block = nullptr;
		is_owner = false;
	}

	UIPtr &setOwner(bool ownership)
	{
		is_owner = ownership;
		return *this;
	}
	bool isOwner() const { return is_owner; }

	void deleteForce()
	{
		is_owner = true;
		clear();
	}

private:
	template<typename OtherType>
	friend class UIPtr;

	void increment_counter()
	{
		if (ptr_control_block)
			Unigine::AtomicAdd(&ptr_control_block->counter, 1);
	}

	void decrement_counter()
	{
		if (ptr_control_block)
		{
			if (is_owner && !ptr_control_block->is_deleted)
			{
				// remove UI element if we have ownership (is unique_ptr)
				ptr_control_block->is_deleted = true;
				ptr->getNode().deleteLater();
				ptr = nullptr;
				is_owner = false;
			}

			const int old_counter = Unigine::AtomicAdd(&ptr_control_block->counter, -1);
			if (old_counter == 1 && ptr_control_block->is_deleted)
			{
				// remove control block if we are the last user of it
				// (and ui element already deleted)
				delete ptr_control_block;
				ptr_control_block = nullptr;
				ptr = nullptr;
				is_owner = false;
			}
		}
	}

	T *ptr;
	UIPtrControlBlock *ptr_control_block;
	bool is_owner{false};
};

template<typename PtrType, typename OtherType>
bool operator==(const UIPtr<PtrType> &lhs, const OtherType &rhs)
{
	return lhs.get() == rhs;
}
template<typename PtrType, typename OtherType>
bool operator==(const OtherType &lhs, const UIPtr<PtrType> &rhs)
{
	return lhs == rhs.get();
}
template<typename PtrType, typename OtherType>
bool operator!=(const UIPtr<PtrType> &lhs, const OtherType &rhs)
{
	return lhs.get() != rhs;
}
template<typename PtrType, typename OtherType>
bool operator!=(const OtherType &lhs, const UIPtr<PtrType> &rhs)
{
	return lhs != rhs.get();
}
template<typename LeftType, typename RightType>
bool operator==(const UIPtr<LeftType> &lhs, const UIPtr<RightType> &rhs)
{
	return lhs.get() == rhs.get();
}
template<typename LeftType, typename RightType>
bool operator!=(const UIPtr<LeftType> &lhs, const UIPtr<RightType> &rhs)
{
	return lhs.get() != rhs.get();
}
template<typename LeftType, typename RightType>
bool operator<(const UIPtr<LeftType> &lhs, const UIPtr<RightType> &rhs)
{
	return lhs.get() < rhs.get();
}

template<typename To, typename From>
UIPtr<To> static_ptr_cast(const UIPtr<From> &ptr)
{
	return UIPtr<To>(static_cast<To *>(ptr.get()));
}
template<typename To, typename From>
UIPtr<To> dynamic_ptr_cast(const UIPtr<From> &ptr)
{
	return UIPtr<To>(dynamic_cast<To *>(ptr.get()));
}

//////////////////////////////////////////////////////////////
// Element
//////////////////////////////////////////////////////////////

class ICanvas;

class Element : public Unigine::ComponentBase
{
public:
	COMPONENT(Element, ComponentBase);
	PROP_NAME("UI_Element");
	COMPONENT_INIT(init, -999);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_GROUP("Element");
	PROP_PARAM(Vec4, pos, "Position");
	PROP_PARAM(Vec2, pivot, "Pivot");
	PROP_PARAM(Vec4, anchor, "Anchor");
	PROP_PARAM(Int, order_offset, 0, "Order");
	PROP_PARAM(Toggle, interactable, 1, "Interactable");
	virtual void applyPropertyChanges();
	void applyPropertyChangesRecursively();
	virtual void applyNodeHierarchyChanges(bool save_screen_position = true);

	// smart pointer
	UIPtr<Element> getPtr() { return UIPtr<Element>(this); }

	// force initialization
	virtual void initializeElement();
	bool isElementInitialized() const { return element_initialized; }

	// canvas
	ICanvas *getCanvas() const { return canvas; }

	// name
	void setName(const char *name) { node->setName(name); }
	const char *getName() const { return node->getName(); }

	// hierarchy
	void setParent(Element *element);
	Element *getParent() const { return parent; }
	virtual void addChild(Element *element);
	void removeChild(Element *element);
	int getNumChildren() const { return children.size(); }
	Element *getChild(int num) const { return children[num]; }
	void setChildIndex(Element *element, int index);
	int getChildIndex(Element *element) const;
	Element *findChild(const char *name, bool recursive = true) const;

	// z-order
	void setOrderOffset(int offset);
	int getOrderOffset() const { return order_offset.get(); }
	int getOrder() const { return get_parent_order() + 1 + order_offset.get(); }

	// position and size (these methods ignore pivot and anchors)
	void setWorldPosition(float left, float top, float right, float bottom);
	void setWorldPosition(
		const Unigine::Math::vec2 &left_top_pos, const Unigine::Math::vec2 &right_bottom_pos);
	void setWorldPosition(const Unigine::Math::vec4 &pos);
	Unigine::Math::vec4 getWorldPosition(bool round_position = true) const;
	void setPositionLeftTop(const Unigine::Math::vec2 &pos);
	void setPositionLeftTop(float x, float y);
	Unigine::Math::vec2 getPositionLeftTop() const;
	void setSize(float width, float height);
	void setSize(float size);
	void setSize(const Unigine::Math::vec2 &size);
	Unigine::Math::vec2 getSize() const;

	// bound (normalized)
	const Unigine::Math::vec2 &getNormalizedBoundMin() const { return min_n; }
	const Unigine::Math::vec2 &getNormalizedBoundMax() const { return max_n; }

	// pivot
	void setPivot(float x, float y);
	void setPivotX(float x);
	void setPivotY(float y);
	void setPivot(const Unigine::Math::vec2 &in_pivot);
	Unigine::Math::vec2 getPivot() const { return pivot; }
	float getPivotX() const { return pivot.get().x; }
	float getPivotY() const { return pivot.get().y; }

	// anchor
	void setAnchor(float x_min, float y_min, float x_max, float y_max);
	void setAnchor(const Unigine::Math::vec4 &in_anchor);
	Unigine::Math::vec4 getAnchor() const { return anchor; }
	bool isFixedWidth() const;
	bool isFixedHeight() const;

	// anchor helpers (fixed width and height)
	void setAnchorLeftTop(bool change_pivot = true);
	void setAnchorLeftMiddle(bool change_pivot = true);
	void setAnchorLeftBottom(bool change_pivot = true);
	void setAnchorCenterTop(bool change_pivot = true);
	void setAnchorCenterMiddle(bool change_pivot = true);
	void setAnchorCenterBottom(bool change_pivot = true);
	void setAnchorRightTop(bool change_pivot = true);
	void setAnchorRightMiddle(bool change_pivot = true);
	void setAnchorRightBottom(bool change_pivot = true);

	// position and size (works only if width/height are fixed!)
	void setPosition(const Unigine::Math::vec2 &position);
	void setPosition(float x, float y);
	void setPositionX(float x);
	void setPositionY(float y);
	Unigine::Math::vec2 getPosition() const;
	float getPositionX() const { return pos.get().x; }
	float getPositionY() const { return pos.get().y; }
	void setWidth(float width);
	void setHeight(float height);
	float getWidth() const { return pos.get().z; }
	float getHeight() const { return pos.get().w; }

	// anchor helpers (non-fixed, width and/or height relative to parent's size)
	void setAnchorExpand();
	void setAnchorStretchHTop(bool change_pivot_y = true);
	void setAnchorStretchHMiddle(bool change_pivot_y = true);
	void setAnchorStretchHBottom(bool change_pivot_y = true);
	void setAnchorStretchVLeft(bool change_pivot_x = true);
	void setAnchorStretchVCenter(bool change_pivot_x = true);
	void setAnchorStretchVRight(bool change_pivot_x = true);

	// position and size (works only if width/height are non-fixed!)
	void setLeftOffset(float offset);
	void setRightOffset(float offset);
	void setTopOffset(float offset);
	void setBottomOffset(float offset);
	float getLeftOffset() const { return pos.get().x; }
	float getRightOffset() const { return pos.get().z; }
	float getTopOffset() const { return pos.get().y; }
	float getBottomOffset() const { return pos.get().w; }

	// interaction
	void setInteractable(bool enabled) { interactable = enabled ? 1 : 0; }
	bool isInteractable() const { return interactable.get() != 0; }
	virtual bool isHover(int screen_x, int screen_y) const;
	virtual bool isHover(int screen_x0, int screen_y0, int screen_x1, int screen_y1) const;

	// events
	Unigine::Event<Element *> &getEventArrange() { return arrange_event; }
	Unigine::Event<Element *> &getEventMouseHoverEnter() { return mouse_hover_enter_event; }
	Unigine::Event<Element *> &getEventMouseHoverExit() { return mouse_hover_exit_event; }
	Unigine::Event<Element *> &getEventMouseClickEnter() { return mouse_click_enter_event; }
	Unigine::Event<Element *> &getEventMouseClickExit() { return mouse_click_exit_event; }
	Unigine::Event<Element *> &getEventDestroy() { return destroy_event; }

protected:
	template<typename OtherType>
	friend class UIPtr;
	friend class ClipMask;
	friend class Canvas;

	virtual void init();
	virtual void shutdown();

	void on_enable() override;
	void on_disable() override;

	// recursive methods that affect all children
	void update_hierarchy(float ifps);
	void update_enabled_hierarchy();
	virtual void set_canvas_hierarchy(UI::ICanvas *canvas);
	void update_order_hierarchy();
	virtual void arrange_hierarchy();
	void update_bound();

	int get_parent_order() const { return (parent ? parent->getOrder() : -1); }
	Unigine::GuiPtr get_gui() const;
	Unigine::WidgetVBoxPtr get_parent_widget() const;

	// helpers
	bool is_in_render_area(int screen_x, int screen_y) const;
	bool is_in_render_area(int screen_x0, int screen_y0, int screen_x1, int screen_y1) const;
	bool is_rect_rect_intersection(int screen_x0, int screen_y0, int screen_x1, int screen_y1,
		const Unigine::Math::vec2 &r1_pos, const Unigine::Math::mat2 &r1_rot,
		const Unigine::Math::vec2 &r1_size) const;
	int get_screen_x() const;
	int get_screen_y() const;
	int get_screen_width() const;
	int get_screen_height() const;

	// virtual methods
	virtual void update(float ifps) {}
	virtual void arrange() {}
	virtual void apply_order_to_widgets() {}
	virtual void set_gui(const Unigine::GuiPtr &gui) {}

	UIPtrControlBlock *ptr_control_block = new UIPtrControlBlock();

	bool element_initialized = false;

	ICanvas *canvas = nullptr;
	Element *parent = nullptr;
	Unigine::Vector<Element *> children;

	// position and size of the element (normalized)
	Unigine::Math::vec2 min_n = Unigine::Math::vec2_zero;
	Unigine::Math::vec2 max_n = Unigine::Math::vec2_one;

	// events
	Unigine::EventInvoker<Element *> arrange_event;
	Unigine::EventInvoker<Element *> mouse_hover_enter_event;
	Unigine::EventInvoker<Element *> mouse_hover_exit_event;
	Unigine::EventInvoker<Element *> mouse_click_enter_event;
	Unigine::EventInvoker<Element *> mouse_click_exit_event;
	Unigine::EventInvoker<Element *> destroy_event;
};
typedef UIPtr<Element> ElementPtr;

//////////////////////////////////////////////////////////////
// Derived Elements
//////////////////////////////////////////////////////////////

class ElementWidget : public Element
{
public:
	ElementWidget(const Unigine::NodePtr &node, int num)
		: Element(node, num)
	{}

protected:
	void lock_arrange();
	bool is_arrange_locked();
	bool unlock_arrange();	  // return true if needed to call arrange()

	enum class ArrangeBehavior : char {
		INSTANT,
		DEFERRED,
		NEED_TO_ARRANGE
	} arrange_behavior = ArrangeBehavior::INSTANT;
};

class ElementFocusable : public ElementWidget
{
public:
	ElementFocusable(const Unigine::NodePtr &node, int num)
		: ElementWidget(node, num)
	{}

	// clang-format off
	PROP_GROUP("Focus");
	PROP_PARAM(Toggle, active, 1, "Active");
	PROP_PARAM(Toggle, focusable, 1, "Focusable");

	PROP_PARAM(Switch, navigation_mode, 0, "Auto,Manual", "Navigation");
	PROP_PARAM(Node, navigation_up, "Element Up", "", "Focus", "navigation_mode=1");
	PROP_PARAM(Node, navigation_down, "Element Down", "", "Focus", "navigation_mode=1");
	PROP_PARAM(Node, navigation_left, "Element Left", "", "Focus", "navigation_mode=1");
	PROP_PARAM(Node, navigation_right, "Element Right", "", "Focus", "navigation_mode=1");

	PROP_PARAM(Toggle, animate_color, 1, "Animate Color");
	PROP_PARAM(Toggle, animate_texture, 0, "Animate Texture");
	PROP_PARAM(Toggle, animate_size, 0, "Animate Size");
	PROP_PARAM(Float, animation_duration, 0.25f, "Animation Duration");

	PROP_PARAM(Color, hover_color, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "Hover Color", "", "Focus", "animate_color=1");
	PROP_PARAM(Color, press_color, Unigine::Math::vec4(0.5f, 0.5f, 0.5f, 1.0f), "Press Color", "", "Focus", "animate_color=1");
	PROP_PARAM(Color, focus_color, Unigine::Math::vec4(1.0f, 0.96f, 0.88f, 1.0f), "Focus Color", "", "Focus", "animate_color=1");
	PROP_PARAM(Color, inactive_color, Unigine::Math::vec4(0.8f, 0.8f, 0.8f, 0.8f), "Inactive Color", "", "Focus", "animate_color=1");

	PROP_PARAM(File, hover_texture, "", "Hover Texture", "", "Focus", "animate_texture=1");
	PROP_PARAM(File, press_texture, "", "Press Texture", "", "Focus", "animate_texture=1");
	PROP_PARAM(File, focus_texture, "", "Focus Texture", "", "Focus", "animate_texture=1");
	PROP_PARAM(File, inactive_texture, "", "Inactive Texture", "", "Focus", "animate_texture=1");

	PROP_PARAM(Switch, scale_animation, 8, "Lerp,Quad,Cubic,Quart,Quint,Expo,Sine,Circ,Back,Elastic,Bounce", "Scale Animation", "", "Focus", "animate_size=1");
	PROP_PARAM(Float, hover_scale, 1.1f, "Hover Scale", "", "Focus", "animate_size=1");
	PROP_PARAM(Float, press_scale, 0.95f, "Press Scale", "", "Focus", "animate_size=1");
	PROP_PARAM(Float, focus_scale, 1.1f, "Focus Scale", "", "Focus", "animate_size=1");
	PROP_PARAM(Float, inactive_scale, 1.0f, "Inactive Scale", "", "Focus", "animate_size=1");
	// clang-format on

	virtual bool isNavigationHorizontallyEnabled() const { return true; }
	virtual bool isNavigationVerticallyEnabled() const { return true; }

	enum class State { Normal, Hover, Press, Focus, Inactive };
	State getState() const { return state; }
	float getStateTransitionTime() const { return transition_percent; }

	virtual void setActive(bool enabled);
	bool isActive() const { return active.get() != 0; }

	virtual void setFocusable(bool value);
	bool isFocusable() const { return focusable.get() != 0; }

	virtual void setFocus(bool focused);
	bool isFocus() const { return focus; }

	// interaction
	virtual void click() = 0;
	Unigine::Event<ElementFocusable *> &getEventStateChanged() { return change_state_event; }

protected:
	bool focus = false;

	State state = State::Normal;
	float transition_percent = 1;
	Unigine::Math::vec4 prev_color = Unigine::Math::vec4_white;
	float prev_scale = 1;

	void set_state(bool active, bool hovering, bool clicking);
	void set_state(State state, bool instant = false);
	void update_state(float ifps);
	Unigine::Math::vec4 get_color(
		const Unigine::Math::vec4 &normal_color = Unigine::Math::vec4_white) const;
	const char *get_texture(const char *normal_texture = nullptr) const;
	float get_scale(float normal_scale = 1.0f) const;

	// events
	Unigine::EventInvoker<ElementFocusable *> change_state_event;
};

//////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////

class MaterialDefaultVariablesSetter
{
public:
	void setMaterial(const Unigine::MaterialPtr &mat);
	void setTextureSize(const Unigine::Math::vec3 &texture_size);
	void setSpritePosAndSize(
		const Unigine::WidgetPtr &widget, const Unigine::Math::ivec2 &screen_size);
	void setMousePosition(const Unigine::WidgetPtr &widget, const Unigine::Math::ivec2 &mouse_pos);
	void runExpressionUpdate(const Unigine::WidgetPtr &widget);

private:
	Unigine::MaterialPtr mat;
	int p_texture_size = -1;	// Int2 texture_size
	int p_sprite_pos = -1;		// Int2 sprite_pos
	int p_sprite_size = -1;		// Int2 sprite_size
	int p_screen_size = -1;		// Int2 screen_size
	int p_mouse_pos = -1;		// Float2 mouse_pos (relative to widget)
	bool has_update_expression = false;
	bool has_post_expression = false;

	Unigine::EventConnection connection;
};

}	 // namespace UI
