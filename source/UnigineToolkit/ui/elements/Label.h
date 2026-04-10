#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class Label : public ElementWidget
{
public:
	COMPONENT(Label, ElementWidget);
	PROP_NAME("UI_Label");

	// clang-format off
	PROP_GROUP("Label");
	PROP_PARAM(String, text, "Label", "Text");
	PROP_PARAM(Float, font_height, 20, "Font Height");
	PROP_PARAM(Color, font_color, Unigine::Math::vec4_white, "Font Color");
	PROP_PARAM(Switch, h_align, 0, "LEFT,CENTER,RIGHT", "Horizontal Align");
	PROP_PARAM(Switch, v_align, 0, "TOP,MIDDLE,BOTTOM", "Vertical Align");
	PROP_PARAM(Float, h_spacing, 0, "Horizontal Spacing", "Spacing between text characters");
	PROP_PARAM(Float, v_spacing, 0, "Vertical Spacing", "Spacing between text lines");
	PROP_PARAM(Toggle, word_wrap, 0, "Word Wrap");
	PROP_PARAM(Toggle, font_rich, 0, "Font Rich");
	PROP_PARAM(Toggle, font_outline, 0, "Outline");
	PROP_PARAM(Switch, auto_resize, 0, "Off,Font Height,Element Size", "Auto Resize");
	PROP_PARAM(Float, min_width, 50, "Min Width", "Minimum Width", "Label", "auto_resize=2");
	PROP_PARAM(Float, max_width, 300, "Max Width", "Maximum Width", "Label", "auto_resize=2");
	PROP_PARAM(File, font_file, "", "Font");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<Label> getPtr() { return UIPtr<Label>(this); }

	void setText(const char *text);
	const char *getText() const { return text.get(); }

	void setFontHeight(float height);	 // font size
	float getFontHeight() const { return font_height; }

	void setFontColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getFontColor() const { return font_color; }

	enum class HORIZONTAL_ALIGN { LEFT, CENTER, RIGHT };
	void setHorizontalAlign(HORIZONTAL_ALIGN align);
	HORIZONTAL_ALIGN getHorizontalAlign() const;

	enum class VERTICAL_ALIGN { TOP, MIDDLE, BOTTOM };
	void setVerticalAlign(VERTICAL_ALIGN align);
	VERTICAL_ALIGN getVerticalAlign() const;

	void setTextAlign(HORIZONTAL_ALIGN h, VERTICAL_ALIGN v = VERTICAL_ALIGN::TOP);

	void setFontHSpacing(float spacing);
	float getFontHSpacing() const { return h_spacing; }

	void setFontVSpacing(float spacing);
	float getFontVSpacing() const { return v_spacing; }

	void setWordWrap(bool wrap);
	bool isWordWrap() const { return word_wrap.get() > 0; }

	void setFontRich(bool rich);
	bool isFontRich() const { return font_rich.get() > 0; }

	void setFontOutline(bool outline);
	bool isFontOutline() const { return font_outline.get() > 0; }

	void setFont(const char *font_file);
	const char *getFont() const { return font_file.get(); }

	Unigine::Math::ivec2 getTextRenderSize() const;
	Unigine::Math::ivec2 getTextRenderSize(const char *text) const;
	Unigine::Math::vec2 getTextCanvasSize() const;
	Unigine::Math::vec2 getTextCanvasSize(const char *text) const;

	// autoresize helpers
	void resizeByText();
	void resizeByText(float min_width, float max_width);

	const Unigine::WidgetLabelPtr &getWidgetLabel() const { return label; }

protected:
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void arrange() override;
	void apply_order_to_widgets() override;
	void set_gui(const Unigine::GuiPtr &gui) override;

	void refresh_text();

	Unigine::WidgetLabelPtr label;
	Unigine::String font_file_stored;
};
typedef UIPtr<Label> LabelPtr;
}	 // namespace UI
