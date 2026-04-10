#pragma once

#include "Canvas.h"
#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class ICanvas;

class ClipMask
	: public ElementWidget
	, public ICanvas
{
public:
	COMPONENT(ClipMask, ElementWidget);
	PROP_NAME("UI_ClipMask");
	COMPONENT_INIT(init_canvas, -1000);

	// clang-format off
	PROP_GROUP("ClipMask");
	PROP_PARAM(Material, mask_material, Unigine::Materials::findManualMaterial("ui_clipmask"), "Mask Material");
	PROP_PARAM(File, mask_texture_file, "", "Mask Texture", "", "ClipMask", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Vec4, mask_uv, Unigine::Math::vec4(0, 0, 1, 1), "Mask UV");
	PROP_PARAM(Color, mask_color, Unigine::Math::vec4_white, "Mask Color");
	// clang-format on

	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<ClipMask> getPtr() { return UIPtr<ClipMask>(this); }

	// engine's gui
	const Unigine::GuiPtr &getGui() const override { return gui; }

	void setMaskMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getMaskMaterial() const { return mask_mat; }
	void setMaskTexture(const char *mask_texture_file);
	void setMaskTexture(const Unigine::TexturePtr &texture);
	const char *getMaskTexture() const { return mask_texture_file.get(); }
	void setMaskUV(const Unigine::Math::vec4 &uv);
	void setMaskUV(float u0, float v0, float u1, float v1);
	Unigine::Math::vec4 getMaskUV() const { return mask_uv.get(); }
	void setMaskColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getMaskColor() const { return mask_color; }

	const Unigine::WidgetSpriteShaderPtr &getWidgetMask() const { return mask_sprite; }

public:
	// clang-format off
	// overrides
	void addChild(Element *element) override;

	float getCanvasWidth() const override { return canvas->getCanvasWidth(); }
	float getCanvasHeight() const override { return canvas->getCanvasHeight(); }
	int getScreenWidth() const override { return canvas->getScreenWidth(); }
	int getScreenHeight() const override { return canvas->getScreenHeight(); }
	const Unigine::Math::ivec2 &getScreenSize() const override { return canvas->getScreenSize(); }
	float getCanvasPixelSize() const override { return canvas->getCanvasPixelSize(); }

	int getFontSize(float canvas_height) const override { return canvas->getFontSize(canvas_height); }

	int convertCanvasToScreen(float canvas_position) const override;
	Unigine::Math::ivec2 convertCanvasToScreen(const Unigine::Math::vec2 &canvas_position) const override;
	float convertScreenToCanvas(int screen_position) const override;
	Unigine::Math::vec2 convertScreenToCanvas(const Unigine::Math::ivec2 &screen_position) const override;

	int getRenderPositionX() const override { return get_screen_x(); }
	int getRenderPositionY() const override { return get_screen_y(); }
	int getRenderWidth() const override { return get_screen_width(); }
	int getRenderHeight() const override { return get_screen_height(); }
	Unigine::Math::ivec2 getRenderSize() const override { return Unigine::Math::ivec2(get_screen_width(), get_screen_height()); }

	Unigine::Math::vec2 getMouseCanvasPosition() const override { return canvas->getMouseCanvasPosition(); }
	Unigine::Math::ivec2 getMouseScreenPosition() const override { return canvas->getMouseScreenPosition(); }
	bool isMouseButtonLeftDown() const override { return canvas->isMouseButtonLeftDown(); }
	bool isMouseButtonMiddleDown() const override { return canvas->isMouseButtonMiddleDown(); }
	bool isMouseButtonRightDown() const override { return canvas->isMouseButtonRightDown(); }
	bool isMouseButtonLeftPressed() const override { return canvas->isMouseButtonLeftPressed(); }
	bool isMouseButtonMiddlePressed() const override { return canvas->isMouseButtonMiddlePressed(); }
	bool isMouseButtonRightPressed() const override { return canvas->isMouseButtonRightPressed(); }
	bool isMouseButtonLeftUp() const override { return canvas->isMouseButtonLeftUp(); }
	bool isMouseButtonMiddleUp() const override { return canvas->isMouseButtonMiddleUp(); }
	bool isMouseButtonRightUp() const override { return canvas->isMouseButtonRightUp(); }
	int getMouseScroll() const override { return canvas->getMouseScroll(); }
	bool isMouseDragging() const override { return canvas->isMouseDragging(); }

	const Unigine::Math::ivec2 &getNavigationDir() const override { return canvas->getNavigationDir(); }
	bool isNavigationEnter() const override { return canvas->isNavigationEnter(); }

	void needToRearrange() override { if (canvas) canvas->needToRearrange(); }
	// clang-format on

protected:
	void init_canvas();
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void update(float ifps) override;
	void arrange() override;
	void apply_order_to_widgets() override;
	void set_canvas_hierarchy(UI::ICanvas *canvas) override;
	void set_gui(const Unigine::GuiPtr &gui) override;

	// callbacks from elements
	void on_add_element(Element *element) override { canvas->on_add_element(element); }
	void on_remove_element(Element *element) override { canvas->on_remove_element(element); }
	void on_focus_changed(ElementFocusable *element) override { canvas->on_focus_changed(element); }

	Unigine::GuiPtr gui;

	Unigine::WidgetSpriteShaderPtr mask_sprite;
	Unigine::RenderTargetPtr render_target;
	Unigine::TexturePtr render_texture;
	Unigine::TexturePtr mask_texture;
	Unigine::Math::vec3 mask_sprite_size;
	Unigine::MaterialPtr mask_mat;
	MaterialDefaultVariablesSetter mask_mat_vars;
};
typedef UIPtr<ClipMask> ClipMaskPtr;
}	 // namespace UI
