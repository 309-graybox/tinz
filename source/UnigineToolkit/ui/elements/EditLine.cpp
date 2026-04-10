#include "EditLine.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(EditLine);

void EditLine::init()
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

	editline_w = WidgetEditLine::create(get_gui(), text);
	editline_w->setBackground(0);
	get_parent_widget()->addChild(editline_w, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();

	editline_w->getEventChanged().connect(
		*this, [this](const WidgetPtr &) { text = editline_w->getText(); });

	getEventMouseHoverEnter().connect(*this, [this](Element *) { hovering = true; });
	getEventMouseHoverExit().connect(*this, [this](Element *) { hovering = false; });
	getEventMouseClickEnter().connect(*this, [this](Element *) {
		clicking = true;
		if (active.get() == 1 && isFocusable())
		{
			setFocus(true);
			get_gui()->focusGained();
		}
	});
	getEventMouseClickExit().connect(*this, [this](Element *) {
		clicking = false;
		if (active.get() == 1)
		{
			clicked_event.run(this);
		}
	});
}

void EditLine::shutdown()
{
	editline_w.deleteLater();
	sprite_w.deleteLater();
	mat.deleteLater();

	Element::shutdown();
}

void EditLine::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setActive(active.get() != 0);

	setFont(font_file);
	setText(text);
	setFontPercent(font_percent);
	setFontColor(font_color);
	setFontOutline(font_outline.get() == 1);

	setBackgroundMaterial(bg_material);
	setBackgroundTexture(bg_texture_file);
	setBackgroundUV(bg_uv);
	setBackgroundColor(bg_color);

	if (unlock_arrange())
		arrange();
}

void EditLine::setActive(bool enabled)
{
	ElementFocusable::setActive(enabled);
	editline_w->setEnabled(enabled);
}

void EditLine::setFocus(bool focused)
{
	ElementFocusable::setFocus(focused);

	if (!focus)
	{
		editline_w->removeFocus();
		editline_w->setSelection(editline_w->getCursor());
	}
}

void EditLine::setText(const char *in_text)
{
	editline_w->setText(in_text);
	arrange();
}

void EditLine::setFontPercent(float value)
{
	font_percent = value;
	arrange();
}

void EditLine::setFontColor(const vec4 &color)
{
	font_color = color;
	editline_w->setFontColor(font_color);
}

void EditLine::setFontOutline(bool outline)
{
	font_outline = outline ? 1 : 0;
	editline_w->setFontOutline(outline ? 1 : 0);
}

void EditLine::setFont(const char *font)
{
	if (font_file_stored == font)
		return;

	font_file = font;
	font_file_stored = font;
	editline_w->setFont(font);
}

void EditLine::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
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

void EditLine::setBackgroundTexture(const char *texture_path)
{
	bg_texture_file = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(sprite_w->getTexture(), texture_path) == 0)
		return;

	default_texture = texture_path;

	sprite_w->setWidth(0);
	sprite_w->setHeight(0);
	sprite_w->setTexture(texture_path);
	sprite_w->arrange();
	sprite_size = vec3(itof(sprite_w->getWidth()), itof(sprite_w->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	mat_vars.setTextureSize(sprite_size);
}

void EditLine::setBackgroundTexture(const TexturePtr &texture)
{
	sprite_w->setRender(texture);
	sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	mat_vars.setTextureSize(sprite_size);
}

void EditLine::setBackgroundUV(const vec4 &uv)
{
	bg_uv = uv;
	sprite_w->setLayerTexCoord(0, uv);
}

void EditLine::setBackgroundUV(float u0, float v0, float u1, float v1)
{
	bg_uv = vec4(u0, v0, u1, v1);
	sprite_w->setLayerTexCoord(0, bg_uv);
}

void EditLine::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	sprite_w->setColor(bg_color);
}

void EditLine::click()
{
	if (active.get() == 0)
		return;

	if (isFocusable())
		setFocus(true);

	set_state(State::Press, true);
	apply_state_animation();

	if (!editline_w->isFocused())
	{
		get_gui()->focusGained();
		editline_w->setFocus();
		editline_w->setCursor(static_cast<int>(strlen(editline_w->getText())));
	}
	else
	{
		editline_w->removeFocus();
	}

	clicked_event.run(this);
}

void EditLine::on_enable()
{
	Element::on_enable();
	if (sprite_w)
	{
		sprite_w->setHidden(false);
		editline_w->setHidden(false);
	}
}

void EditLine::on_disable()
{
	if (sprite_w)
	{
		sprite_w->setHidden(true);
		editline_w->setHidden(true);
	}
	Element::on_disable();
}

void EditLine::update(float ifps)
{
	// update material
	mat_vars.setMousePosition(sprite_w, canvas->getMouseScreenPosition());
	mat_vars.runExpressionUpdate(sprite_w);

	// animation
	if (clicking && !canvas->isMouseButtonLeftPressed())
		clicking = false;
	if (Engine::get()->isEditorLoaded())
	{
		hovering = false;
		clicking = false;
	}

	set_state(active.get() != 0, hovering, clicking);
	update_state(ifps);
	apply_state_animation();
}

void EditLine::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int s_x = get_screen_x();
	int s_y = get_screen_y();
	int s_w = max(1, get_screen_width());
	int s_h = max(1, get_screen_height());

	// arrange sprite
	sprite_w->setPosition(s_x, s_y);
	sprite_w->setWidth(s_w);
	sprite_w->setHeight(s_h);
	sprite_w->arrange();
	mat_vars.setSpritePosAndSize(sprite_w, canvas->getScreenSize());

	// arrange label
	float font_height = (max_n.y - min_n.y) * canvas->getCanvasHeight() * font_percent;
	editline_w->setFontSize(canvas->getFontSize(font_height));
	editline_w->setWidth(s_w);
	editline_w->setHeight(s_h);
	editline_w->setPosition(s_x, s_y);
	editline_w->arrange();
}

void EditLine::apply_order_to_widgets()
{
	if (!sprite_w)
		return;
	sprite_w->setOrder(getOrder());
	editline_w->setOrder(getOrder());

	WidgetPtr parent = sprite_w->getParent();
	parent->removeChild(sprite_w);
	parent->addChild(sprite_w);
	parent->removeChild(editline_w);
	parent->addChild(editline_w);
}

void EditLine::set_gui(const Unigine::GuiPtr &gui)
{
	if (!sprite_w)
		return;
	if (gui != sprite_w->getParentGui())
	{
		gui->addChild(sprite_w);
		gui->addChild(editline_w);
	}
}

void EditLine::apply_state_animation()
{
	if (animate_color.get() == 1)
		sprite_w->setColor(get_color(bg_color));
	if (animate_texture.get() == 1)
	{
		const char *texture = get_texture(default_texture);
		if (strcmp(sprite_w->getTexture(), texture) != 0)
			sprite_w->setTexture(texture);
	}
	if (animate_size.get() == 1)
	{
		float scale = get_scale();
		int x = get_screen_x();
		int y = get_screen_y();
		int w = max(1, get_screen_width());
		int h = max(1, get_screen_height());
		int o = ftoi(h * scale - h);
		x -= o / 2;
		y -= o / 2;
		w += o;
		h += o;
		sprite_w->setPosition(x, y);
		sprite_w->setWidth(w);
		sprite_w->setHeight(h);
		sprite_w->arrange();
		mat_vars.setSpritePosAndSize(sprite_w, canvas->getScreenSize());
	}
}
