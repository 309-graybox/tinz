#include "ProgressBar.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(ProgressBar);

void ProgressBar::init()
{
	if (element_initialized)
		return;

	lock_arrange();
	Element::init();
	unlock_arrange();

	// create
	bg_sprite_w = WidgetSpriteShader::create(get_gui(), "white.texture");
	bg_sprite_w->arrange();
	bg_sprite_size = vec3(itof(bg_sprite_w->getWidth()), itof(bg_sprite_w->getHeight()), 0);
	get_parent_widget()->addChild(bg_sprite_w, Gui::ALIGN_OVERLAP);

	fg_sprite_w = WidgetSpriteShader::create(get_gui(), "white.texture");
	fg_sprite_w->arrange();
	fg_sprite_size = vec3(itof(fg_sprite_w->getWidth()), itof(fg_sprite_w->getHeight()), 0);
	get_parent_widget()->addChild(fg_sprite_w, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();
}

void ProgressBar::shutdown()
{
	bg_sprite_w.deleteLater();
	bg_mat.deleteLater();
	fg_sprite_w.deleteLater();
	fg_mat.deleteLater();

	Element::shutdown();
}

void ProgressBar::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setBackgroundMaterial(bg_material);
	setBackgroundTexture(bg_texture);
	setBackgroundColor(bg_color);

	setForegroundMaterial(fg_material);
	setForegroundTexture(fg_texture);
	setForegroundColor(fg_color);

	setValue(value);

	if (unlock_arrange())
		arrange();
}

void ProgressBar::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
{
	if (!in_material && !bg_mat)
		return;

	if (bg_mat && bg_mat->getParent() == in_material)
		return;

	bg_mat.deleteLater();
	if (in_material)
	{
		bg_mat = in_material->inherit();

		bg_mat_vars.setMaterial(bg_mat);
		bg_mat_vars.setTextureSize(bg_sprite_size);
		bg_mat_vars.setSpritePosAndSize(bg_sprite_w, canvas->getScreenSize());
		bg_sprite_w->setMaterial(bg_mat);
	}
	else
	{
		bg_mat_vars.setMaterial(nullptr);
		bg_sprite_w->setMaterial(nullptr);
	}
}

void ProgressBar::setBackgroundTexture(const char *texture_path)
{
	bg_texture = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(bg_sprite_w->getTexture(), texture_path) == 0)
		return;

	bg_default_texture = texture_path;

	bg_sprite_w->setWidth(0);
	bg_sprite_w->setHeight(0);
	bg_sprite_w->setTexture(texture_path);
	bg_sprite_w->arrange();
	bg_sprite_size = vec3(itof(bg_sprite_w->getWidth()), itof(bg_sprite_w->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	bg_mat_vars.setTextureSize(bg_sprite_size);
}

void ProgressBar::setBackgroundTexture(const TexturePtr &texture)
{
	bg_sprite_w->setRender(texture);
	bg_sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	bg_mat_vars.setTextureSize(bg_sprite_size);
}

void ProgressBar::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	bg_sprite_w->setColor(bg_color);
}

void ProgressBar::setForegroundMaterial(const Unigine::MaterialPtr &in_material)
{
	if (!in_material && !fg_mat)
		return;

	if (fg_mat && fg_mat->getParent() == in_material)
		return;

	fg_mat.deleteLater();
	if (in_material)
	{
		fg_mat = in_material->inherit();

		fg_mat_vars.setMaterial(fg_mat);
		fg_mat_vars.setTextureSize(fg_sprite_size);
		fg_mat_vars.setSpritePosAndSize(fg_sprite_w, canvas->getScreenSize());
		fg_sprite_w->setMaterial(fg_mat);
	}
	else
	{
		fg_mat_vars.setMaterial(nullptr);
		fg_sprite_w->setMaterial(nullptr);
	}
}

void ProgressBar::setForegroundTexture(const char *texture_path)
{
	fg_texture = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(fg_sprite_w->getTexture(), texture_path) == 0)
		return;

	fg_default_texture = texture_path;

	fg_sprite_w->setWidth(0);
	fg_sprite_w->setHeight(0);
	fg_sprite_w->setTexture(texture_path);
	fg_sprite_w->arrange();
	fg_sprite_size = vec3(itof(fg_sprite_w->getWidth()), itof(fg_sprite_w->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	fg_mat_vars.setTextureSize(fg_sprite_size);
}

void ProgressBar::setForegroundTexture(const TexturePtr &texture)
{
	fg_sprite_w->setRender(texture);
	fg_sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	fg_mat_vars.setTextureSize(fg_sprite_size);
}

void ProgressBar::setForegroundColor(const Unigine::Math::vec4 &color)
{
	fg_color = color;
	fg_sprite_w->setColor(fg_color);
}

void ProgressBar::setValue(float in_value)
{
	float new_value = saturate(in_value);
	if (value == new_value)
		return;

	value = new_value;
	arrange();

	changed_event.run(this);
}

const Unigine::WidgetSpriteShaderPtr &ProgressBar::getWidgetSpriteShaderBackground() const
{
	return bg_sprite_w;
}

const Unigine::WidgetSpriteShaderPtr &ProgressBar::getWidgetSpriteShaderForeground() const
{
	return fg_sprite_w;
}

void ProgressBar::on_enable()
{
	Element::on_enable();
	if (bg_sprite_w)
	{
		bg_sprite_w->setHidden(false);
		fg_sprite_w->setHidden(false);
	}
}

void ProgressBar::on_disable()
{
	if (bg_sprite_w)
	{
		bg_sprite_w->setHidden(true);
		fg_sprite_w->setHidden(true);
	}
	Element::on_disable();
}

void ProgressBar::update(float ifps)
{
	// update material
	bg_mat_vars.setMousePosition(bg_sprite_w, canvas->getMouseScreenPosition());
	bg_mat_vars.runExpressionUpdate(bg_sprite_w);
	fg_mat_vars.setMousePosition(fg_sprite_w, canvas->getMouseScreenPosition());
	fg_mat_vars.runExpressionUpdate(fg_sprite_w);
}

void ProgressBar::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int s_x = get_screen_x();
	int s_y = get_screen_y();
	int s_w = max(1, get_screen_width());
	int s_h = max(1, get_screen_height());

	// arrange background
	bg_sprite_w->setPosition(s_x, s_y);
	bg_sprite_w->setWidth(s_w);
	bg_sprite_w->setHeight(s_h);
	bg_sprite_w->arrange();
	bg_mat_vars.setSpritePosAndSize(bg_sprite_w, canvas->getScreenSize());

	// arrange foreground
	int fg_w = ftoi(value.get() * s_w);
	if (fg_w > 0)
	{
		fg_sprite_w->setHidden(false);
		fg_sprite_w->setPosition(s_x, s_y);
		fg_sprite_w->setWidth(fg_w);
		fg_sprite_w->setHeight(s_h);
		fg_sprite_w->arrange();
		fg_mat_vars.setSpritePosAndSize(fg_sprite_w, canvas->getScreenSize());
	}
	else
		fg_sprite_w->setHidden(true);
}

void ProgressBar::apply_order_to_widgets()
{
	if (!bg_sprite_w)
		return;
	bg_sprite_w->setOrder(getOrder());
	fg_sprite_w->setOrder(getOrder());

	WidgetPtr parent = bg_sprite_w->getParent();
	parent->removeChild(bg_sprite_w);
	parent->addChild(bg_sprite_w);
	parent->removeChild(fg_sprite_w);
	parent->addChild(fg_sprite_w);
}

void ProgressBar::set_gui(const Unigine::GuiPtr &gui)
{
	if (!bg_sprite_w)
		return;
	if (gui != bg_sprite_w->getParentGui())
	{
		gui->addChild(bg_sprite_w);
		gui->addChild(fg_sprite_w);
	}
}
