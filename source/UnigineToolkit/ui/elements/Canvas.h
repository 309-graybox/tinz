#pragma once
#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {
class ICanvas
{
public:
	virtual const Unigine::GuiPtr &getGui() const = 0;
	virtual float getCanvasWidth() const = 0;
	virtual float getCanvasHeight() const = 0;
	virtual int getScreenWidth() const = 0;
	virtual int getScreenHeight() const = 0;
	virtual const Unigine::Math::ivec2 &getScreenSize() const = 0;
	virtual float getCanvasPixelSize() const = 0;

	virtual int getFontSize(float canvas_height) const = 0;

	virtual int convertCanvasToScreen(float canvas_position) const = 0;
	virtual Unigine::Math::ivec2 convertCanvasToScreen(
		const Unigine::Math::vec2 &canvas_position) const = 0;
	virtual float convertScreenToCanvas(int screen_position) const = 0;
	virtual Unigine::Math::vec2 convertScreenToCanvas(
		const Unigine::Math::ivec2 &screen_position) const = 0;

	virtual int getRenderPositionX() const = 0;
	virtual int getRenderPositionY() const = 0;
	virtual int getRenderWidth() const = 0;
	virtual int getRenderHeight() const = 0;
	virtual Unigine::Math::ivec2 getRenderSize() const = 0;

	virtual Unigine::Math::vec2 getMouseCanvasPosition() const = 0;
	virtual Unigine::Math::ivec2 getMouseScreenPosition() const = 0;
	virtual bool isMouseButtonLeftDown() const = 0;
	virtual bool isMouseButtonMiddleDown() const = 0;
	virtual bool isMouseButtonRightDown() const = 0;
	virtual bool isMouseButtonLeftPressed() const = 0;
	virtual bool isMouseButtonMiddlePressed() const = 0;
	virtual bool isMouseButtonRightPressed() const = 0;
	virtual bool isMouseButtonLeftUp() const = 0;
	virtual bool isMouseButtonMiddleUp() const = 0;
	virtual bool isMouseButtonRightUp() const = 0;
	virtual int getMouseScroll() const = 0;
	virtual bool isMouseDragging() const = 0;

	virtual const Unigine::Math::ivec2 &getNavigationDir() const = 0;
	virtual bool isNavigationEnter() const = 0;

	virtual void needToRearrange() = 0;

	// callbacks from elements
	virtual void on_add_element(Element *element) = 0;
	virtual void on_remove_element(Element *element) = 0;
	virtual void on_focus_changed(ElementFocusable *element) = 0;
};

class Canvas
	: public Element
	, public ICanvas
{
public:
	// clang-format off

	COMPONENT(Canvas, Element);
	PROP_NAME("UI_Canvas");
	COMPONENT_INIT(init_canvas, -1000);
	COMPONENT_POST_UPDATE(post_update, 1000);

	// overrides
	PROP_GROUP("Element");
	PROP_PARAM(Vec4, pos, Unigine::Math::vec4(0, 0, 0, 0), "Position");		// zero offset from borders
	PROP_PARAM(Vec2, pivot, Unigine::Math::vec2(0.5f, 0.5f), "Pivot");		// center
	PROP_PARAM(Vec4, anchor, Unigine::Math::vec4(0, 0, 1, 1), "Anchor");	// expand

	PROP_GROUP("Canvas");
	// Constant Pixel Size - "as is"
	// Reference Resolution - using reference for scaling UI element
	PROP_PARAM(Switch, canvas_mode, 1, "Constant Pixel Size,Reference Resolution", "Canvas Mode");
	PROP_PARAM(Toggle, use_dpi_scale, 1, "Use System DPI Scale", "", "Canvas", "canvas_mode=0");
	PROP_PARAM(Float, pixel_scale, 1, "Pixel Scale", "", "Canvas", "canvas_mode=0");
	PROP_PARAM(Float, reference_width, 1920, "Reference Width", "", "Canvas", "canvas_mode=1");
	PROP_PARAM(Float, reference_height, 1080, "Reference Height", "", "Canvas", "canvas_mode=1");
	PROP_PARAM(Switch, update_mode, 0, "Auto (PostUpdate),Manual", "Update Mode");
	PROP_PARAM(Switch, render_mode, 0, "Screen,Texture", "Render Mode");

	PROP_PARAM(Material, material, "Material", "", "Canvas", "render_mode=1");
	PROP_PARAM(String, texture_name, "albedo", "Texture Name", "", "Canvas", "render_mode=1");
	PROP_PARAM(Int, texture_width, 512, "Texture Width", "", "Canvas", "render_mode=1");
	PROP_PARAM(Int, texture_height, 512, "Texture Height", "", "Canvas", "render_mode=1");

	void applyNodeHierarchyChanges(bool save_screen_position = true) override {}

	// get any canvas with render_mode == "Screen"
	static Canvas *get() { return instance; }
	static const Unigine::Vector<Canvas *> &getAllCanvases() { return canvases; }

	// smart pointer
	UIPtr<Canvas> getPtr() { return UIPtr<Canvas>(this); }

	// force initialization
	void initializeElement() override;

	// main loop
	void needToRearrange() override;	// rearrange widgets in the next updateManual()
	void updateManual(float ifps);		// input is disabled, only render
	void updateManual(const Unigine::Math::vec2 &mouse_pos, int mouse_button, int mouse_scroll,
		const Unigine::Math::ivec2 &navigation_dir, bool navigation_enter, float ifps);

	const Unigine::TexturePtr &render(int width, int height, const Unigine::Math::vec4 &bg_color = Unigine::Math::vec4_black);
	const Unigine::TexturePtr &render(int width, int height, const Unigine::TexturePtr &bg_texture);
	const Unigine::TexturePtr &render(int width, int height, const Unigine::PlayerPtr &bg_camera,
		const Unigine::Math::vec4 &bg_fade_color = Unigine::Math::vec4_zero);
	const Unigine::TexturePtr &getRenderTexture() const { return render_texture; }

	// engine's gui
	const Unigine::GuiPtr &getGui() const override { return gui; }

	// screen info (render area)
	int getScreenWidth() const override { return screen_size.x; }
	int getScreenHeight() const override { return screen_size.y; }
	const Unigine::Math::ivec2 &getScreenSize() const override { return screen_size; }

	// canvas scale info
	float getCanvasWidth() const override { return canvas_width; }
	float getCanvasHeight() const override { return canvas_height; }
	float getCanvasReferenceWidth() const { return reference_width; }
	float getCanvasReferenceHeight() const { return reference_height; }
	float getCanvasPixelSize() const override;

	// font info
	float getCanvasFontHeight(int size) const;
	float getScreenFontHeight(int size) const;
	int getFontSize(float canvas_height) const override;
	int getFontSize(int screen_height) const;

	// canvas converters
	int convertCanvasToScreen(float canvas_position) const override;
	Unigine::Math::ivec2 convertCanvasToScreen(const Unigine::Math::vec2 &canvas_position) const override;
	float convertScreenToCanvas(int screen_position) const override;
	Unigine::Math::vec2 convertScreenToCanvas(const Unigine::Math::ivec2 &screen_position) const override;

	// render info
	int getRenderPositionX() const override { return 0; }
	int getRenderPositionY() const override { return 0; }
	int getRenderWidth() const override { return screen_size.x; }
	int getRenderHeight() const override { return screen_size.y; }
	Unigine::Math::ivec2 getRenderSize() const override { return screen_size; }

	// cursor info
	Unigine::Math::vec2 getMouseCanvasPosition() const override { return mouse_pos; }
	Unigine::Math::ivec2 getMouseScreenPosition() const override { return convertCanvasToScreen(mouse_pos); }
	bool isMouseButtonLeftDown() const override { return (mouse_button_down & 1) != 0; }
	bool isMouseButtonMiddleDown() const override { return (mouse_button_down & (1 << 1)) != 0; }
	bool isMouseButtonRightDown() const override { return (mouse_button_down & (1 << 2)) != 0; }
	bool isMouseButtonLeftPressed() const override { return (mouse_button_pressed & 1) != 0; }
	bool isMouseButtonMiddlePressed() const override { return (mouse_button_pressed & (1 << 1)) != 0; }
	bool isMouseButtonRightPressed() const override { return (mouse_button_pressed & (1 << 2)) != 0; }
	bool isMouseButtonLeftUp() const override { return (mouse_button_up & 1) != 0; }
	bool isMouseButtonMiddleUp() const override { return (mouse_button_up & (1 << 1)) != 0; }
	bool isMouseButtonRightUp() const override { return (mouse_button_up & (1 << 2)) != 0; }
	bool isMouseDragging() const override { return mouse_dragging; }
	int getMouseScroll() const override { return mouse_scroll; }

	const Unigine::Math::ivec2 &getNavigationDir() const override { return nav_dir; }
	bool isNavigationEnter() const override { return nav_enter; }

	Element *getHoveredElement() const { return hovered_element.get(); }
	Element *getElement(int screen_x, int screen_y) const;
	Element *getElement(const Unigine::Math::vec2 &canvas_pos) const;
	ElementFocusable *getFocus() const;

	// events
	Unigine::Event<int /*width*/, int /*height*/> &getResizeEvent() { return resize_event; }
	Unigine::Event<> &getBeforeRenderGuiEvent() { return before_render_gui_event; }

	// clang-format on

private:
	void init_canvas();
	void init() override;
	void shutdown() override;
	void post_update();

	void calculate_font_heights();
	bool refresh_canvas_size();
	void update_cursor(const Unigine::Math::vec2 &mouse_pos, int mouse_button, int mouse_scroll);
	void update_navigation(Unigine::Math::ivec2 navigation_dir, bool navigation_enter);

	void init_material();
	void render_to_material();
	void shutdown_material();

	bool is_render_mode_screen() const;
	bool is_update_mode_manual() const;

	// navigation
	ElementFocusable *get_first_focusable_element() const;

	// callbacks from elements
	void on_add_element(Element *element) override;
	void on_remove_element(Element *element) override;
	void on_focus_changed(ElementFocusable *element) override;

	static Canvas *instance;
	static Unigine::Vector<Canvas *> canvases;

	Unigine::GuiPtr gui;
	Unigine::EventConnections window_connections;

	// canvas variables
	float canvas_width = 1920;
	float canvas_height = 1080;
	Unigine::Math::ivec2 screen_size;
	float system_dpi_scale = 1.0f;
	bool need_to_rearrange = true;

	// render for the Editor
	Unigine::RenderTargetPtr render_target;
	Unigine::TexturePtr render_texture;

	// font info
	static Unigine::Curve2dPtr font_height_to_size;		// size = evaluate(height)
	static Unigine::Vector<int> font_size_to_height;	// font_height[getFontSize()]

	// mouse info
	Unigine::Math::vec2 mouse_pos;
	int mouse_button_down = 0;
	int mouse_button_pressed = 0;
	int mouse_button_up = 0;
	int mouse_scroll = 0;
	bool mouse_dragging = false;
	Unigine::Input::MOUSE_HANDLE saved_mouse_handle = Unigine::Input::MOUSE_HANDLE_USER;

	// navigation info
	bool nav_enter = false;
	Unigine::Math::ivec2 nav_dir;

	// interaction and navigation
	UIPtr<Element> hovered_element;
	UIPtr<ElementFocusable> focused_element;
	Unigine::Vector<ElementFocusable *> focusable_elements;

	// events
	Unigine::EventInvoker<int, int> resize_event;
	Unigine::EventInvoker<> before_render_gui_event;

	// render to texture
	int texture_id = -1;
};
typedef UIPtr<Canvas> CanvasPtr;
}	 // namespace UI
