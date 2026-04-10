#pragma once

#include "ClipMask.h"
#include "Scroll.h"

#include <UnigineComponentSystem.h>

namespace UI {

class ScrollBox : public ElementWidget
{
public:
	COMPONENT(ScrollBox, ElementWidget);
	PROP_NAME("UI_ScrollBox");

	// clang-format off
	PROP_GROUP("ScrollBox");
	PROP_PARAM(Node, viewport_node, "Viewport");
	PROP_PARAM(Node, content_node, "Content");
	PROP_PARAM(Switch, horizontal_scroll_mode, 1, "None,Auto Hide,Always Visible", "Horizontal Scroll Mode");
	PROP_PARAM(Node, horizontal_scroll_node, "Horizontal Scroll");
	PROP_PARAM(Switch, vertical_scroll_mode, 1, "Node,Auto Hide,Always Visible", "Vertical Scroll Mode");
	PROP_PARAM(Node, vertical_scroll_node, "Vertical Scroll");
	
	PROP_PARAM(Material, bg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Material");
	PROP_PARAM(File, bg_texture_param, GET_GUID("UnigineToolkit/ui/textures/scrollbox_bg.png"), "Texture", "", "ScrollBox", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Vec4, bg_uv, Unigine::Math::vec4(0, 0, 1, 1), "UV");
	PROP_PARAM(Color, bg_color, Unigine::Math::vec4_white, "Color");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<ScrollBox> getPtr() { return UIPtr<ScrollBox>(this); }

	const ClipMaskPtr &getViewport() const { return viewport; }
	const ElementPtr &getContent() const { return content; }
	const ScrollPtr &getHorizontalScroll() const { return horizontal_scroll; }
	const ScrollPtr &getVerticalScroll() const { return vertical_scroll; }

	enum class SCROLL_MODE { NONE, AUTO_HIDE, ALWAYS_VISIBLE };
	void setHorizontalScrollMode(SCROLL_MODE mode);
	SCROLL_MODE getHorizontalScrollMode() const;

	void setVerticalScrollMode(SCROLL_MODE mode);
	SCROLL_MODE getVerticalScrollMode() const;

	void setBackgroundMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getBackgroundMaterial() const { return w_mat; }

	void setBackgroundTexture(const char *texture_path);
	void setBackgroundTexture(const Unigine::TexturePtr &texture);
	const char *getBackgroundTexture() const { return bg_texture_param.get(); }

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

	void subscribe_to_viewport();
	void subscribe_to_horizontal_scroll();
	void subscribe_to_vertical_scroll();

	Unigine::WidgetSpriteShaderPtr w_sprite;
	Unigine::Math::vec3 w_sprite_size;
	Unigine::MaterialPtr w_mat;
	MaterialDefaultVariablesSetter w_mat_vars;

	ClipMaskPtr viewport;
	ElementPtr content;
	ScrollPtr horizontal_scroll;
	ScrollPtr vertical_scroll;

	bool moving = false;
	Unigine::Math::vec2 mouse_start_pos;
};
typedef UIPtr<ScrollBox> ScrollBoxPtr;
}	 // namespace UI
