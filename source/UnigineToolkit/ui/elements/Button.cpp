#include "Button.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Button);

void Button::init()
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

	label = WidgetLabel::create(get_gui(), text);
	get_parent_widget()->addChild(label, Gui::ALIGN_OVERLAP);

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
		if (active.get() == 1 && isFocusable())
			setFocus(true);
	});
	getEventMouseClickExit().connect(*this, [this](Element *) {
		clicking = false;
		if (active.get() == 1)
		{
			clicked_event.run(this);
		}
	});
}

void Button::shutdown()
{
	label.deleteLater();
	sprite_w.deleteLater();
	mat.deleteLater();

	Element::shutdown();
}

void Button::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setFont(font_file);
	setText(text);
	setFontPercent(font_percent);
	setFontColor(font_color);
	setHorizontalAlign(static_cast<HORIZONTAL_ALIGN>(font_align.get()));
	setFontOffset(font_offset);
	setFontOutline(font_outline.get() == 1);

	setBackgroundMaterial(bg_material);
	setBackgroundTexture(bg_texture_file);
	setBackgroundUV(bg_uv);
	setBackgroundColor(bg_color);

	if (unlock_arrange())
		arrange();
}

void Button::setText(const char *in_text)
{
	text = in_text;
	StringStack<> t = Localization::get(text);
	t = t.replace("\\n", "\n");
	label->setText(t);
	arrange();
}

void Button::setFontPercent(float value)
{
	font_percent = value;
	arrange();
}

void Button::setFontColor(const vec4 &color)
{
	font_color = color;
	label->setFontColor(font_color);
}

void Button::setHorizontalAlign(HORIZONTAL_ALIGN align)
{
	font_align = static_cast<int>(align);
	switch (align)
	{
	case HORIZONTAL_ALIGN::LEFT:
		label->setTextAlign(Gui::ALIGN_LEFT);
		break;
	case HORIZONTAL_ALIGN::CENTER:
		label->setTextAlign(Gui::ALIGN_CENTER);
		break;
	case HORIZONTAL_ALIGN::RIGHT:
		label->setTextAlign(Gui::ALIGN_RIGHT);
		break;
	}
	arrange();
}

Button::HORIZONTAL_ALIGN Button::getHorizontalAlign() const
{
	return static_cast<Button::HORIZONTAL_ALIGN>(font_align.get());
}

void Button::setFontOffset(const Unigine::Math::vec2 &offset)
{
	font_offset = offset;
	arrange();
}

void Button::setFontOutline(bool outline)
{
	font_outline = outline ? 1 : 0;
	label->setFontOutline(outline ? 1 : 0);
}

void Button::setFont(const char *font)
{
	if (font_file_stored == font)
		return;

	font_file = font;
	font_file_stored = font;
	label->setFont(font);
}

void Button::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
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

void Button::setBackgroundTexture(const char *texture_path)
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

void Button::setBackgroundTexture(const TexturePtr &texture)
{
	sprite_w->setRender(texture);
	sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	mat_vars.setTextureSize(sprite_size);
}

void Button::setBackgroundUV(const vec4 &uv)
{
	bg_uv = uv;
	sprite_w->setLayerTexCoord(0, uv);
}

void Button::setBackgroundUV(float u0, float v0, float u1, float v1)
{
	bg_uv = vec4(u0, v0, u1, v1);
	sprite_w->setLayerTexCoord(0, bg_uv);
}

void Button::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	sprite_w->setColor(bg_color);
}

void Button::click()
{
	if (active.get() == 0)
		return;

	if (isFocusable())
		setFocus(true);

	set_state(State::Press, true);
	apply_state_animation();

	clicked_event.run(this);
}

void Button::on_enable()
{
	Element::on_enable();
	if (sprite_w)
	{
		sprite_w->setHidden(false);
		label->setHidden(false);
	}
}

void Button::on_disable()
{
	if (sprite_w)
	{
		sprite_w->setHidden(true);
		label->setHidden(true);
	}
	Element::on_disable();
}

void Button::update(float ifps)
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

void Button::arrange()
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
	label->setFontSize(canvas->getFontSize(font_height));
	label->setWidth(s_w);
	label->arrange();
	int x = s_x;
	int y = s_y + s_h / 2 - label->getHeight() / 2;
	switch (font_align.get())
	{
	case static_cast<int>(HORIZONTAL_ALIGN::LEFT):
	case static_cast<int>(HORIZONTAL_ALIGN::CENTER):
		x += canvas->convertCanvasToScreen(font_offset.get().x);
		break;
	case static_cast<int>(HORIZONTAL_ALIGN::RIGHT):
		x -= canvas->convertCanvasToScreen(font_offset.get().x);
		break;
	default:	// nothing to do
		break;
	}
	y += canvas->convertCanvasToScreen(font_offset.get().y);
	label->setPosition(x, y);
}

void Button::apply_order_to_widgets()
{
	if (!sprite_w)
		return;
	sprite_w->setOrder(getOrder());
	label->setOrder(getOrder());

	WidgetPtr parent = sprite_w->getParent();
	parent->removeChild(sprite_w);
	parent->addChild(sprite_w);
	parent->removeChild(label);
	parent->addChild(label);
}

void Button::set_gui(const Unigine::GuiPtr &gui)
{
	if (!sprite_w)
		return;
	if (gui != sprite_w->getParentGui())
	{
		gui->addChild(sprite_w);
		gui->addChild(label);
	}
}

void Button::apply_state_animation()
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
