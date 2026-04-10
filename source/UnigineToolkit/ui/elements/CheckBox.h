#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class CheckBox : public ElementFocusable
{
public:
	COMPONENT(CheckBox, ElementFocusable);
	PROP_NAME("UI_CheckBox");

	// clang-format off
	PROP_GROUP("CheckBox");
	PROP_PARAM(Toggle, checked, 0, "Checked");
	PROP_PARAM(String, text, "CheckBox", "Text");
	PROP_PARAM(Float, font_percent, 0.8f, "Font Percent");
	PROP_PARAM(Vec2, font_offset, "Font Offset");
	PROP_PARAM(Color, font_color, Unigine::Math::vec4_white, "Font Color");
	PROP_PARAM(Toggle, font_outline, 1, "Font Outline");
	PROP_PARAM(File, font_file, "", "Font");

	PROP_PARAM(Material, bg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Material");
	PROP_PARAM(File, bg_texture_file, GET_GUID("UnigineToolkit/ui/textures/button.png"), "Texture", "", "CheckBox", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Vec4, bg_uv, Unigine::Math::vec4(0, 0, 1, 1), "UV");
	PROP_PARAM(Color, bg_color, Unigine::Math::vec4_white, "Color");

	PROP_PARAM(File, checkmark_texture_file, GET_GUID("UnigineToolkit/ui/textures/checkmark.png"), "Checkmark Texture", "", "CheckBox", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Float, checkmark_percent, 1.0f, "Checkmark Percent");
	// clang-format on

	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<CheckBox> getPtr() { return UIPtr<CheckBox>(this); }

	void setChecked(bool checked);
	bool isChecked() const { return checked.get() != 0; }

	// label text
	void setText(const char *text);
	const char *getText() const { return text.get(); }

	void setFontPercent(float value);
	float getFontPercent() const { return font_percent; }

	void setFontOffset(const Unigine::Math::vec2 &offset);
	Unigine::Math::vec2 getFontOffset() const { return font_offset; }

	void setFontColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getFontColor() const { return font_color; }

	void setFontOutline(bool outline);
	bool isFontOutline() const { return font_outline.get() > 0; }

	void setFont(const char *font_file);
	const char *getFont() const { return font_file.get(); }

	// background image
	void setBackgroundMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getBackgroundMaterial() const { return mat; }

	void setBackgroundTexture(const char *texture_path);
	void setBackgroundTexture(const Unigine::TexturePtr &texture);
	const char *getBackgroundTexture() const { return bg_texture_file.get(); }

	void setBackgroundUV(const Unigine::Math::vec4 &uv);
	void setBackgroundUV(float u0, float v0, float u1, float v1);
	Unigine::Math::vec4 getBackgroundUV() const { return bg_uv; }

	void setBackgroundColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getBackgroundColor() const { return bg_color; }

	// checkmark image
	void setCheckmarkTexture(const char *texture_path);
	void setCheckmarkTexture(const Unigine::TexturePtr &texture);
	const char *getCheckmarkTexture() const { return checkmark_texture_file.get(); }

	void setCheckmarkPercent(float value);
	float getCheckmarkPercent() const { return checkmark_percent; }

	// interaction
	void click() override;
	Unigine::Event<CheckBox *> &getEventCheckBoxChanged() { return changed_event; }

	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShader() const { return sprite_w; }
	const Unigine::WidgetLabelPtr &getWidgetLabel() const { return label; }

protected:
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void update(float ifps) override;
	void arrange() override;
	void apply_order_to_widgets() override;
	void set_gui(const Unigine::GuiPtr &gui) override;

	void apply_state_animation();

	Unigine::WidgetSpriteShaderPtr sprite_w;
	Unigine::Math::vec3 sprite_size;
	Unigine::String default_texture;
	Unigine::MaterialPtr mat;
	MaterialDefaultVariablesSetter mat_vars;

	Unigine::WidgetSpritePtr sprite_chkmark;

	Unigine::WidgetLabelPtr label;
	Unigine::String font_file_stored;

	bool hovering = false;
	bool clicking = false;

	// events
	Unigine::EventInvoker<CheckBox *> changed_event;
};
typedef UIPtr<CheckBox> CheckBoxPtr;
}	 // namespace UI
