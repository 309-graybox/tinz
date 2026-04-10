#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class Scroll : public ElementFocusable
{
public:
	COMPONENT(Scroll, ElementFocusable);
	PROP_NAME("UI_Scroll");

	// clang-format off
	PROP_GROUP("Scroll");
	PROP_PARAM(Material, bg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Background Material");
	PROP_PARAM(File, bg_texture, GET_GUID("UnigineToolkit/ui/textures/button_pressed.png"), "Background Texture", "", "Scroll", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, bg_color, Unigine::Math::vec4_white, "Background Color");

	PROP_PARAM(Material, handle_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Handle Material");
	PROP_PARAM(File, handle_texture, GET_GUID("UnigineToolkit/ui/textures/button.png"), "Handle Texture", "", "Scroll", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, handle_color, Unigine::Math::vec4_white, "Handle Color");

	PROP_PARAM(Switch, direction, 0, "Left to Right,Right to Left,Top to Bottom,Bottom to Top", "Direction");
	PROP_PARAM(Float, value, 0, "Value");
	PROP_PARAM(Float, scroll_size, 0.25f, "Size");
	PROP_PARAM(Int, number_of_steps, 0, "Number of Steps");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<Scroll> getPtr() { return UIPtr<Scroll>(this); }

	bool isNavigationHorizontallyEnabled() const override;
	bool isNavigationVerticallyEnabled() const override;

	// background image
	void setBackgroundMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getBackgroundMaterial() const { return bg_mat; }
	void setBackgroundTexture(const char *texture_path);
	void setBackgroundTexture(const Unigine::TexturePtr &texture);
	const char *getBackgroundTexture() const { return bg_texture.get(); }
	void setBackgroundColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getBackgroundColor() const { return bg_color; }

	// handle image
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
	void setValue(float value);
	float getValue() const { return value.get(); }
	void setScrollSize(float value);
	float getScrollSize() const { return scroll_size.get(); }
	void setNumberOfSteps(int steps);
	int getNumberOfSteps() const { return number_of_steps.get(); }

	// interaction
	void click() override;
	Unigine::Event<Scroll *> &getEventScrollChanged() { return changed_event; }

	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShaderBackground() const;
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

	Unigine::WidgetSpriteShaderPtr h_sprite_w;
	Unigine::Math::vec3 h_sprite_size;
	Unigine::String h_default_texture;
	Unigine::MaterialPtr h_mat;
	MaterialDefaultVariablesSetter h_mat_vars;

	bool hovering = false;
	bool clicking = false;
	int click_offset = 0;

	// events
	Unigine::EventInvoker<Scroll *> changed_event;
};
typedef UIPtr<Scroll> ScrollPtr;
}	 // namespace UI
