#include "Scroll.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Scroll);

void Scroll::init()
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

	h_sprite_w = WidgetSpriteShader::create(get_gui(), "white.texture");
	h_sprite_w->arrange();
	h_sprite_size = vec3(itof(h_sprite_w->getWidth()), itof(h_sprite_w->getHeight()), 0);
	get_parent_widget()->addChild(h_sprite_w, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();

	getEventMouseHoverEnter().connect(*this, [this](Element *) { hovering = true; });
	getEventMouseHoverExit().connect(*this, [this](Element *) { hovering = false; });
	getEventMouseClickEnter().connect(*this, [this](Element *) {
		clicking = true;
		if (active.get() == 1)
		{
			setFocus(true);

			// calc mouse position relative to handle (to prevent unintentional set value)
			ivec2 p = canvas->getMouseScreenPosition();
			if (direction.get() < 2 /*horizontal*/)
			{
				if (p.x >= h_sprite_w->getScreenPositionX()
					&& p.x < (h_sprite_w->getScreenPositionX() + h_sprite_w->getWidth()))
					click_offset =
						p.x - h_sprite_w->getScreenPositionX() - h_sprite_w->getWidth() / 2;
				else
					click_offset = 0;
			}
			else	// vertical
			{
				if (p.y >= h_sprite_w->getScreenPositionY()
					&& p.y < (h_sprite_w->getScreenPositionY() + h_sprite_w->getHeight()))
					click_offset =
						p.y - h_sprite_w->getScreenPositionY() - h_sprite_w->getHeight() / 2;
				else
					click_offset = 0;
			}
		}
	});
	getEventMouseClickExit().connect(*this, [this](Element *) { clicking = false; });
}

void Scroll::shutdown()
{
	bg_sprite_w.deleteLater();
	bg_mat.deleteLater();
	h_sprite_w.deleteLater();
	h_mat.deleteLater();

	Element::shutdown();
}

void Scroll::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setBackgroundMaterial(bg_material);
	setBackgroundTexture(bg_texture);
	setBackgroundColor(bg_color);

	setHandleMaterial(handle_material);
	setHandleTexture(handle_texture);
	setHandleColor(handle_color);

	setDirection(static_cast<DIRECTION>(direction.get()));
	setValue(value);
	setScrollSize(scroll_size);
	setNumberOfSteps(number_of_steps);

	if (unlock_arrange())
		arrange();
}

bool Scroll::isNavigationHorizontallyEnabled() const
{
	if (direction.get() < 2 /*horizontal*/)
		return !isFocus();
	return true;
}

bool Scroll::isNavigationVerticallyEnabled() const
{
	if (direction.get() >= 2 /*vertical*/)
		return !isFocus();
	return true;
}

void Scroll::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
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

void Scroll::setBackgroundTexture(const char *texture_path)
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

void Scroll::setBackgroundTexture(const TexturePtr &texture)
{
	bg_sprite_w->setRender(texture);
	bg_sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	bg_mat_vars.setTextureSize(bg_sprite_size);
}

void Scroll::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	bg_sprite_w->setColor(bg_color);
}

void Scroll::setHandleMaterial(const Unigine::MaterialPtr &in_material)
{
	if (!in_material && !h_mat)
		return;

	if (h_mat && h_mat->getParent() == in_material)
		return;

	h_mat.deleteLater();
	if (in_material)
	{
		h_mat = in_material->inherit();

		h_mat_vars.setMaterial(h_mat);
		h_mat_vars.setTextureSize(h_sprite_size);
		h_mat_vars.setSpritePosAndSize(h_sprite_w, canvas->getScreenSize());
		h_sprite_w->setMaterial(h_mat);
	}
	else
	{
		h_mat_vars.setMaterial(nullptr);
		h_sprite_w->setMaterial(nullptr);
	}
}

void Scroll::setHandleTexture(const char *texture_path)
{
	handle_texture = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(h_sprite_w->getTexture(), texture_path) == 0)
		return;

	h_default_texture = texture_path;

	h_sprite_w->setWidth(0);
	h_sprite_w->setHeight(0);
	h_sprite_w->setTexture(texture_path);
	h_sprite_w->arrange();
	h_sprite_size = vec3(itof(h_sprite_w->getWidth()), itof(h_sprite_w->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	h_mat_vars.setTextureSize(h_sprite_size);
}

void Scroll::setHandleTexture(const TexturePtr &texture)
{
	h_sprite_w->setRender(texture);
	h_sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	h_mat_vars.setTextureSize(h_sprite_size);
}

void Scroll::setHandleColor(const Unigine::Math::vec4 &color)
{
	handle_color = color;
	h_sprite_w->setColor(handle_color);
}

void Scroll::setDirection(DIRECTION dir)
{
	direction = static_cast<int>(dir);
	arrange();
}

Scroll::DIRECTION Scroll::getDirection() const
{
	return static_cast<Scroll::DIRECTION>(direction.get());
}

void Scroll::setValue(float in_value)
{
	float new_value = saturate(in_value);
	if (value == new_value)
		return;

	value = new_value;
	arrange();

	changed_event.run(this);
}

void Scroll::setScrollSize(float in_size)
{
	scroll_size = saturate(in_size);
	arrange();
}

void Scroll::setNumberOfSteps(int steps)
{
	number_of_steps = steps;
	arrange();
}

void Scroll::click()
{
	if (active.get() == 0)
		return;

	if (isFocusable())
		setFocus(true);

	set_state(State::Press, true);
	apply_state_animation();
}

const Unigine::WidgetSpriteShaderPtr &Scroll::getWidgetSpriteShaderBackground() const
{
	return bg_sprite_w;
}

const Unigine::WidgetSpriteShaderPtr &Scroll::getWidgetSpriteShaderHandle() const
{
	return h_sprite_w;
}

void Scroll::on_enable()
{
	Element::on_enable();
	if (bg_sprite_w)
	{
		bg_sprite_w->setHidden(false);
		h_sprite_w->setHidden(false);
	}
}

void Scroll::on_disable()
{
	if (bg_sprite_w)
	{
		bg_sprite_w->setHidden(true);
		h_sprite_w->setHidden(true);
	}
	Element::on_disable();
}

void Scroll::update(float ifps)
{
	// update material
	bg_mat_vars.setMousePosition(bg_sprite_w, canvas->getMouseScreenPosition());
	bg_mat_vars.runExpressionUpdate(bg_sprite_w);
	h_mat_vars.setMousePosition(h_sprite_w, canvas->getMouseScreenPosition());
	h_mat_vars.runExpressionUpdate(h_sprite_w);

	if (clicking && !canvas->isMouseButtonLeftPressed())
		clicking = false;
	if (Engine::get()->isEditorLoaded())
	{
		hovering = false;
		clicking = false;
	}

	// set value (mouse)
	if (clicking && active.get() == 1 && (click_offset == 0 || canvas->isMouseDragging()))
	{
		ivec2 p = canvas->getMouseScreenPosition();
		float v = 0;
		if (direction.get() < 2 /*horizontal*/)
		{
			int x = bg_sprite_w->getScreenPositionX() + h_sprite_w->getWidth() / 2;
			int w = bg_sprite_w->getWidth() - h_sprite_w->getWidth();
			v = itof(p.x - click_offset - x) / w;
		}
		else	// vertical
		{
			int y = bg_sprite_w->getScreenPositionY() + h_sprite_w->getHeight() / 2;
			int h = bg_sprite_w->getHeight() - h_sprite_w->getHeight();
			v = itof(p.y - click_offset - y) / h;
		}
		if (direction.get() == 1 || direction.get() == 3)	 // reverse
			v = 1.0f - saturate(v);

		int n = number_of_steps - 1;
		setValue(n > 0 ? (Math::round(v * n) / n) : v);
	}

	// set value (keyboard/gamepad)
	if (isFocus() && active.get() == 1)
	{
		int dir = 0;
		switch (getDirection())
		{
		case DIRECTION::LEFT_TO_RIGHT:
			dir = canvas->getNavigationDir().x;
			break;
		case DIRECTION::RIGHT_TO_LEFT:
			dir = -canvas->getNavigationDir().x;
			break;
		case DIRECTION::TOP_TO_BOTTOM:
			dir = -canvas->getNavigationDir().y;
			break;
		case DIRECTION::BOTTOM_TO_TOP:
			dir = canvas->getNavigationDir().y;
			break;
		}
		if (dir != 0)
		{
			int n = number_of_steps;
			if (n != 0)
			{
				setValue(getValue() + dir);
			}
			else
			{
				const int num_steps = 10;
				float step = 1.0f / num_steps;
				setValue(getValue() + dir * step);
			}
		}
	}

	// animation
	set_state(active.get() != 0, hovering || (clicking && canvas->isMouseDragging()), clicking);
	update_state(ifps);
	apply_state_animation();
}

void Scroll::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int s_x = get_screen_x();
	int s_y = get_screen_y();
	int s_w = max(1, get_screen_width());
	int s_h = max(1, get_screen_height());

	float v = value;
	bool reversed = (direction.get() == 1 || direction.get() == 3);
	if (reversed)
		v = 1.0f - saturate(v);

	// arrange background/foreground
	bg_sprite_w->setPosition(s_x, s_y);
	bg_sprite_w->setWidth(s_w);
	bg_sprite_w->setHeight(s_h);
	bg_sprite_w->arrange();
	bg_mat_vars.setSpritePosAndSize(bg_sprite_w, canvas->getScreenSize());

	// arrange handle
	if (direction.get() < 2 /*horizontal*/)
	{
		int h_w = Math::max(1, ftoi(s_w * scroll_size));
		h_sprite_w->setPosition(s_x + ftoi(v * (s_w - h_w)), s_y);
		h_sprite_w->setWidth(h_w);
		h_sprite_w->setHeight(s_h);
		h_sprite_w->arrange();
	}
	else	// vertical
	{
		int h_h = Math::max(1, ftoi(s_h * scroll_size));
		h_sprite_w->setPosition(s_x, s_y + ftoi(v * (s_h - h_h)));
		h_sprite_w->setWidth(s_w);
		h_sprite_w->setHeight(h_h);
		h_sprite_w->arrange();
	}
	h_mat_vars.setSpritePosAndSize(h_sprite_w, canvas->getScreenSize());
}

void Scroll::apply_order_to_widgets()
{
	if (!bg_sprite_w)
		return;
	bg_sprite_w->setOrder(getOrder());
	h_sprite_w->setOrder(getOrder());

	WidgetPtr parent = bg_sprite_w->getParent();
	parent->removeChild(bg_sprite_w);
	parent->addChild(bg_sprite_w);
	parent->removeChild(h_sprite_w);
	parent->addChild(h_sprite_w);
}

void Scroll::set_gui(const Unigine::GuiPtr &gui)
{
	if (!bg_sprite_w)
		return;
	if (gui != bg_sprite_w->getParentGui())
	{
		gui->addChild(bg_sprite_w);
		gui->addChild(h_sprite_w);
	}
}

void Scroll::apply_state_animation()
{
	if (animate_color.get() == 1)
		h_sprite_w->setColor(get_color(handle_color));
	if (animate_texture.get() == 1)
	{
		const char *texture = get_texture(h_default_texture);
		if (strcmp(h_sprite_w->getTexture(), texture) != 0)
			h_sprite_w->setTexture(texture);
	}
	if (animate_size.get() == 1)
	{
		float scale = get_scale();
		float v = value;
		bool reversed = (direction.get() == 1 || direction.get() == 3);
		if (reversed)
			v = 1.0f - saturate(v);

		int x = get_screen_x();
		int y = get_screen_y();
		int w = max(1, get_screen_width());
		int h = max(1, get_screen_height());

		if (direction.get() < 2 /*horizontal*/)
		{
			int o = ftoi(h * scale - h);
			int h_w = Math::max(1, ftoi(w * scroll_size));
			h_sprite_w->setPosition(x + ftoi(v * (w - h_w)) - o / 2, y - o / 2);
			h_sprite_w->setWidth(Math::max(1, h_w + o));
			h_sprite_w->setHeight(Math::max(1, h + o));
		}
		else	// vertical
		{
			int o = ftoi(w * scale - w);
			int h_h = Math::max(1, ftoi(h * scroll_size));
			h_sprite_w->setPosition(x - o / 2, y + ftoi(v * (h - h_h)) - o / 2);
			h_sprite_w->setWidth(Math::max(1, w + o));
			h_sprite_w->setHeight(Math::max(1, h_h + o));
		}
		h_sprite_w->arrange();
		h_mat_vars.setSpritePosAndSize(h_sprite_w, canvas->getScreenSize());
	}
}
