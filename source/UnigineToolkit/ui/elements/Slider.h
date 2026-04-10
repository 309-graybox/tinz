#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class Slider : public ElementFocusable
{
public:
	COMPONENT(Slider, ElementFocusable);
	PROP_NAME("UI_Slider");

	// clang-format off
	PROP_GROUP("Slider");
	PROP_PARAM(Float, bg_height_percent, 0.5f, "Background Height", "Relative background height to handle (in percents, from 0 to 1)");
	PROP_PARAM(Material, bg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Background Material");
	PROP_PARAM(File, bg_texture, GET_GUID("UnigineToolkit/ui/textures/button_pressed.png"), "Background Texture", "", "Slider", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, bg_color, Unigine::Math::vec4_white, "Background Color");

	PROP_PARAM(Material, fg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Foreground Material");
	PROP_PARAM(File, fg_texture, GET_GUID("UnigineToolkit/ui/textures/rectangle.png"), "Foreground Texture", "", "Slider", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, fg_color, Unigine::Math::vec4_white, "Foreground Color");

	PROP_PARAM(Float, handle_width_percent, 0.5f, "Handle Width", "Relative handle width to its height (in percents, from 0 to 1)");
	PROP_PARAM(Material, handle_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Handle Material");
	PROP_PARAM(File, handle_texture, GET_GUID("UnigineToolkit/ui/textures/button.png"), "Handle Texture", "", "Slider", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, handle_color, Unigine::Math::vec4_white, "Handle Color");

	PROP_PARAM(Switch, direction, 0, "Left to Right,Right to Left,Top to Bottom,Bottom to Top", "Direction");
	PROP_PARAM(Float, min_value, 0, "Min Value");
	PROP_PARAM(Float, max_value, 1, "Max Value");
	PROP_PARAM(Toggle, whole_numbers, 0, "Whole Numbers");
	PROP_PARAM(Float, value, 0, "Value");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<Slider> getPtr() { return UIPtr<Slider>(this); }

	bool isNavigationHorizontallyEnabled() const override;
	bool isNavigationVerticallyEnabled() const override;

	// background image
	void setBackgroundHeight(float percent);
	float getBackgroundHeight() const { return bg_height_percent; }
	void setBackgroundMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getBackgroundMaterial() const { return bg_mat; }
	void setBackgroundTexture(const char *texture_path);
	void setBackgroundTexture(const Unigine::TexturePtr &texture);
	const char *getBackgroundTexture() const { return bg_texture.get(); }
	void setBackgroundColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getBackgroundColor() const { return bg_color; }

	// foreground image
	void setForegroundMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getForegroundMaterial() const { return fg_mat; }
	void setForegroundTexture(const char *texture_path);
	void setForegroundTexture(const Unigine::TexturePtr &texture);
	const char *getForegroundTexture() const { return fg_texture.get(); }
	void setForegroundColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getForegroundColor() const { return fg_color; }

	// handle image
	void setHandleWidth(float percent);
	float getHandleWidth() const { return handle_width_percent; }
	void setHandleMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getHandleMaterial() const { return h_mat; }
	void setHandleTexture(const char *texture_path);
	void setHandleTexture(const Unigine::TexturePtr &texture);
	const char *getHandleTexture() const { return handle_texture.get(); }
	void setHandleColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getHandleColor() const { return handle_color; }

	// direction
	enum class DIRECTION { LEFT_TO_RIGHT, RIGHT_TO_LEFT, TOP_TO_BOTTOM, BOTTOM_TO_TOP };
	void setDirection(DIRECTION direction);
	DIRECTION getDirection() const;

	// value
	void setMinValue(float value);
	float getMinValue() const { return min_value.get(); }
	void setMaxValue(float value);
	float getMaxValue() const { return max_value.get(); }
	void setWholeNumbers(bool enabled);
	bool isWholeNumbers() const { return whole_numbers.get() != 0; }
	void setValue(float value);
	float getValue() const { return value.get(); }

	// interaction
	void click() override;
	Unigine::Event<Slider *> &getEventSliderChanged() { return changed_event; }

	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShaderBackground() const;
	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShaderForeground() const;
	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShaderHandle() const;

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

	Unigine::WidgetSpriteShaderPtr bg_sprite_w;
	Unigine::Math::vec3 bg_sprite_size;
	Unigine::String bg_default_texture;
	Unigine::MaterialPtr bg_mat;
	MaterialDefaultVariablesSetter bg_mat_vars;

	Unigine::WidgetSpriteShaderPtr fg_sprite_w;
	Unigine::Math::vec3 fg_sprite_size;
	Unigine::String fg_default_texture;
	Unigine::MaterialPtr fg_mat;
	MaterialDefaultVariablesSetter fg_mat_vars;

	Unigine::WidgetSpriteShaderPtr h_sprite_w;
	Unigine::Math::vec3 h_sprite_size;
	Unigine::String h_default_texture;
	Unigine::MaterialPtr h_mat;
	MaterialDefaultVariablesSetter h_mat_vars;

	bool hovering = false;
	bool clicking = false;
	int click_offset = 0;

	// events
	Unigine::EventInvoker<Slider *> changed_event;
};
typedef UIPtr<Slider> SliderPtr;
}	 // namespace UI
