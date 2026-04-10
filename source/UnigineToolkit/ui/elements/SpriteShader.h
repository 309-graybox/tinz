#pragma once

#include "Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class Canvas;

class SpriteShader : public ElementWidget
{
public:
	COMPONENT(SpriteShader, ElementWidget);
	PROP_NAME("UI_SpriteShader");

	// clang-format off
	PROP_GROUP("SpriteShader");
	PROP_PARAM(Material, material, "Material");
	PROP_PARAM(File, texture_file, "", "Texture", "", "SpriteShader", "filter=.png|.jpg|.jpeg|.tif|.tiff|.tga|.texture");
	PROP_PARAM(Vec4, uv, Unigine::Math::vec4(0, 0, 1, 1), "UV");
	PROP_PARAM(Color, color, Unigine::Math::vec4_white, "Color");
	PROP_PARAM(Float, angle, 0, "Rotation", "Rotation angle in degrees", "SpriteShader", "min=0;max=360");
	PROP_PARAM(Toggle, fixed_ratio, 0, "Fixed Ratio");
	// clang-format on
	void applyPropertyChanges() override;

	// smart pointer
	UIPtr<SpriteShader> getPtr() { return UIPtr<SpriteShader>(this); }

	void setMaterial(const Unigine::MaterialPtr &material);
	const Unigine::MaterialPtr &getMaterial() const { return mat; }

	void setTexture(const char *texture_path);
	void setTexture(const Unigine::TexturePtr &texture);
	const char *getTexture() const { return texture_file.get(); }

	void setUV(const Unigine::Math::vec4 &uv);
	void setUV(float u0, float v0, float u1, float v1);
	Unigine::Math::vec4 getUV() const { return uv; }

	void setColor(const Unigine::Math::vec4 &color);
	Unigine::Math::vec4 getColor() const { return color; }

	void setFixedRatio(bool in_fixed_ratio);
	bool isFixedRatio() const { return fixed_ratio.get() == 1; }

	void setRotation(float angle_deg);
	float getRotation() const { return angle; }

	// interaction
	bool isHover(int screen_x, int screen_y) const override;
	bool isHover(int screen_x0, int screen_y0, int screen_x1, int screen_y1) const override;

	const Unigine::WidgetSpriteShaderPtr &getWidgetSpriteShader() const { return sprite_w; }

protected:
	void init() override;
	void shutdown() override;

	void on_enable() override;
	void on_disable() override;

	void update(float ifps) override;
	void arrange() override;
	void apply_order_to_widgets() override;
	void set_gui(const Unigine::GuiPtr &gui) override;

	Unigine::WidgetSpriteShaderPtr sprite_w;
	Unigine::Math::vec3 sprite_size;
	Unigine::MaterialPtr mat;
	MaterialDefaultVariablesSetter mat_vars;
};
typedef UIPtr<SpriteShader> SpriteShaderPtr;
}	 // namespace UI
