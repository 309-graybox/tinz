#include "SpriteShader.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(SpriteShader);

void SpriteShader::init()
{
	if (element_initialized)
		return;

	lock_arrange();
	Element::init();
	unlock_arrange();

	// create
	sprite_w = WidgetSpriteShader::create(get_gui(), "white.texture");
	sprite_w->arrange();
	sprite_size = vec3(itof(sprite_w->getWidth()), itof(sprite_w->getHeight()), 0);
	get_parent_widget()->addChild(sprite_w, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();
}

void SpriteShader::shutdown()
{
	sprite_w.deleteLater();
	mat.deleteLater();

	Element::shutdown();
}

void SpriteShader::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setMaterial(material);
	setTexture(texture_file);
	setUV(uv);
	setColor(color);
	setRotation(angle);
	setFixedRatio(fixed_ratio.get() == 1);

	if (unlock_arrange())
		arrange();
}

void SpriteShader::setMaterial(const Unigine::MaterialPtr &in_material)
{
	if (!in_material && !mat)
		return;

	if (mat && mat->getParent() == in_material)
		return;

	mat.deleteLater();
	if (in_material)
	{
		mat = in_material->inherit();

		mat_vars.setMaterial(mat);
		mat_vars.setTextureSize(sprite_size);
		mat_vars.setSpritePosAndSize(sprite_w, canvas->getScreenSize());
		sprite_w->setMaterial(mat);
	}
	else
	{
		mat_vars.setMaterial(nullptr);
		sprite_w->setMaterial(nullptr);
	}
}

void SpriteShader::setTexture(const char *texture_path)
{
	texture_file = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(sprite_w->getTexture(), texture_path) == 0)
		return;

	sprite_w->setTransform(mat4_identity);
	sprite_w->setWidth(0);
	sprite_w->setHeight(0);
	sprite_w->setTexture(texture_path);
	sprite_w->arrange();
	sprite_size = vec3(itof(sprite_w->getWidth()), itof(sprite_w->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	mat_vars.setTextureSize(sprite_size);
}

void SpriteShader::setTexture(const TexturePtr &texture)
{
	sprite_w->setRender(texture);
	sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	mat_vars.setTextureSize(sprite_size);
}

void SpriteShader::setUV(const vec4 &in_uv)
{
	uv = in_uv;
	sprite_w->setLayerTexCoord(0, in_uv);
}

void SpriteShader::setUV(float u0, float v0, float u1, float v1)
{
	uv = vec4(u0, v0, u1, v1);
	sprite_w->setLayerTexCoord(0, uv);
}

void SpriteShader::setColor(const Unigine::Math::vec4 &in_color)
{
	color = in_color;
	sprite_w->setColor(in_color);
}

void SpriteShader::setFixedRatio(bool in_fixed_ratio)
{
	if ((fixed_ratio.get() == 1) == in_fixed_ratio)
		return;
	fixed_ratio = in_fixed_ratio ? 1 : 0;
	arrange();
}

void SpriteShader::setRotation(float angle_deg)
{
	if (angle == angle_deg)
		return;
	angle = angle_deg;
	arrange();
}

bool SpriteShader::isHover(int x, int y) const
{
	if (compare(angle, 0.f))
		return Element::isHover(x, y);

	if (!isEnabled() || !is_in_render_area(x, y))
		return false;

	// check point selection
	int w = canvas->getScreenWidth();
	int h = canvas->getScreenHeight();
	vec2 s = vec2((max_n.x - min_n.x) * w, (max_n.y - min_n.y) * h);	// size in screen pixels
	vec2 p = pivot;
	vec2 rel_pos = vec2(x - lerp(min_n.x, max_n.x, p.x) * w, y - lerp(min_n.y, max_n.y, p.y) * h);
	vec2 rot_pos = rotateZ(-angle) * rel_pos + p * s;
	return rot_pos.x >= 0 && rot_pos.x < s.x && rot_pos.y >= 0 && rot_pos.y < s.y;
}

bool SpriteShader::isHover(int x0, int y0, int x1, int y1) const
{
	if (compare(angle, 0.f))
		return Element::isHover(x0, y0, x1, y1);

	if (!isEnabled() || !is_in_render_area(x0, y0, x1, y1))
		return false;

	// check rectangle intersecion
	int w = canvas->getScreenWidth();
	int h = canvas->getScreenHeight();
	vec2 p = pivot;

	float sinv, cosv;
	Math::sincos(angle * Consts::DEG2RAD, sinv, cosv);
	mat2 r1_rot = mat2(cosv, sinv, -sinv, cosv);
	vec2 r1_size = vec2((max_n.x - min_n.x) * w, (max_n.y - min_n.y) * h);
	vec2 r1_pivot_pos = vec2(lerp(min_n.x, max_n.x, p.x) * w, lerp(min_n.y, max_n.y, p.y) * h);
	vec2 r1_pos = r1_pivot_pos + r1_rot * lerp(r1_size, -r1_size, p) * 0.5f;
	return is_rect_rect_intersection(x0, y0, x1, y1, r1_pos, r1_rot, r1_size);
}

void SpriteShader::on_enable()
{
	Element::on_enable();
	if (sprite_w)
		sprite_w->setHidden(false);
}

void SpriteShader::on_disable()
{
	if (sprite_w)
		sprite_w->setHidden(true);
	Element::on_disable();
}

void SpriteShader::update(float ifps)
{
	// update material
	mat_vars.setMousePosition(sprite_w, canvas->getMouseScreenPosition());
	mat_vars.runExpressionUpdate(sprite_w);
}

void SpriteShader::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int s_x = get_screen_x();
	int s_y = get_screen_y();
	int s_w = max(1, get_screen_width());
	int s_h = max(1, get_screen_height());
	vec2 p = pivot;

	if (fixed_ratio)
	{
		float sprite_ratio = sprite_size.x / sprite_size.y;
		float area_ratio = itof(s_w) / itof(s_h);
		if (area_ratio > sprite_ratio)
		{
			float new_w = s_h * sprite_ratio;
			s_x += ftoi((s_w - new_w) * p.x);
			s_w = ftoi(new_w);
		}
		else
		{
			float new_h = s_w / sprite_ratio;
			s_y += ftoi((s_h - new_h) * p.y);
			s_h = ftoi(new_h);
		}
	}

	if (compare(angle, 0.f))
	{
		sprite_w->setPosition(s_x, s_y);
		sprite_w->setWidth(s_w);
		sprite_w->setHeight(s_h);
		sprite_w->setTransform(mat4_identity);
	}
	else
	{
		sprite_w->setPosition(
			s_x + ftoi((s_w - sprite_size.x) * p.x), s_y + ftoi((s_h - sprite_size.y) * p.y));
		sprite_w->setWidth(0);
		sprite_w->setHeight(0);

		vec3 t = vec3(sprite_size.x * p.x, sprite_size.y * p.y, 0);
		sprite_w->setTransform(translate(t) * rotateZ(angle)
							   * scale(s_w / sprite_size.x, s_h / sprite_size.y, 1)
							   * translate(-t));
	}
	sprite_w->arrange();
	mat_vars.setSpritePosAndSize(sprite_w, canvas->getScreenSize());
}

void SpriteShader::apply_order_to_widgets()
{
	if (!sprite_w)
		return;
	sprite_w->setOrder(getOrder());
	WidgetPtr parent = sprite_w->getParent();
	parent->removeChild(sprite_w);
	parent->addChild(sprite_w);
}

void SpriteShader::set_gui(const Unigine::GuiPtr &gui)
{
	if (!sprite_w)
		return;
	if (gui != sprite_w->getParentGui())
		gui->addChild(sprite_w);
}
