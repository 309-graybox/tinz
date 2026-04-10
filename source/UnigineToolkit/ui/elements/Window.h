#pragma once

#include "ClipMask.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Window : public ElementWidget
{
public:
	COMPONENT(Window, ElementWidget);
	PROP_NAME("UI_Window");

	// clang-format off
	PROP_GROUP("Window");
	PROP_PARAM(Node, content_node, "Content");
	PROP_PARAM(Toggle, moveable, 1, "Moveable");
	PROP_PARAM(Toggle, sizeable, 1, "Sizeable");
	PROP_PARAM(Material, bg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Material");
	PROP_PARAM(File, bg_texture_param, GET_GUID("UnigineToolkit/ui/textures/window.png"), "Texture", "", "Window", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(File, bg_texture_sizeable_param, GET_GUID("UnigineToolkit/ui/textures/window_sizeable.png"), "Texture Sizeable", "", "Window", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Vec4, bg_uv, Unigine::Math::vec4(0, 0, 1, 1), "UV");
	PROP_PARAM(Color, bg_color, Unigine::Math::vec4_white, "Color");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<Window> getPtr() { return UIPtr<Window>(this); }

	const ClipMaskPtr &getContent() const { return content; }

	void setMoveable(bool value) { moveable = value ? 1 : 0; }
	bool isMoveable() const { return moveable.get() > 0; }

	void setSizeable(bool value);
	bool isSizeable() const { return sizeable.get() > 0; }

	void setBackgroundMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getBackgroundMaterial() const { return w_mat; }

	void setBackgroundTexture(const char *texture_path);
	void setBackgroundTexture(const Unigine::TexturePtr &texture);
	const char *getBackgroundTexture() const;

	void setBackgroundUV(const Unigine::Math::vec4 &uv);
	void setBackgroundUV(float u0, float v0, float u1, float v1);
	Unigine::Math::vec4 getBackgroundUV() const { return bg_uv; }

	void setBackgroundColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getBackgroundColor() const { return bg_color; }

	const Unigine::WidgetSpriteShaderPtr &getWidgetBackground() const { return w_sprite; }

protected:
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void update(float ifps) override;
	void arrange() override;
	void apply_order_to_widgets() override;
	void set_gui(const Unigine::GuiPtr &gui) override;

	Unigine::WidgetSpriteShaderPtr w_sprite;
	Unigine::Math::vec3 w_sprite_size;
	Unigine::MaterialPtr w_mat;
	MaterialDefaultVariablesSetter w_mat_vars;

	ClipMaskPtr content;

	bool moving = false;
	bool resizing = false;
	Unigine::Math::vec2 mouse_start_pos;
};
typedef UIPtr<Window> WindowPtr;
}	 // namespace UI
