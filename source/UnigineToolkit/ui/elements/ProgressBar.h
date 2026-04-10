#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class ProgressBar : public ElementWidget
{
public:
	COMPONENT(ProgressBar, ElementWidget);
	PROP_NAME("UI_ProgressBar");

	// clang-format off
	PROP_GROUP("ProgressBar");
	PROP_PARAM(Material, bg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Background Material");
	PROP_PARAM(File, bg_texture, GET_GUID("UnigineToolkit/ui/textures/button_pressed.png"), "Background Texture", "", "ProgressBar", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, bg_color, Unigine::Math::vec4(0.5f, 0.5, 0.5f, 1.0f), "Background Color");

	PROP_PARAM(Material, fg_material, Unigine::Materials::findManualMaterial("ui_nine_sliced"), "Foreground Material");
	PROP_PARAM(File, fg_texture, GET_GUID("UnigineToolkit/ui/textures/rectangle.png"), "Foreground Texture", "", "ProgressBar", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Color, fg_color, Unigine::Math::vec4_white, "Foreground Color");

	PROP_PARAM(Float, value, 0.5f, "Value");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<ProgressBar> getPtr() { return UIPtr<ProgressBar>(this); }

	// background image
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

	// value
	void setValue(float value);
	float getValue() const { return value.get(); }

	// interaction
	Unigine::Event<ProgressBar *> &getEventProgressBarChanged() { return changed_event; }

	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShaderBackground() const;
	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShaderForeground() const;

protected:
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void update(float ifps) override;
	void arrange() override;
	void apply_order_to_widgets() override;
	void set_gui(const Unigine::GuiPtr &gui) override;

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

	// events
	Unigine::EventInvoker<ProgressBar *> changed_event;
};
typedef UIPtr<ProgressBar> ProgressBarPtr;
}	 // namespace UI
