#include "CheckBox.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(CheckBox);

void CheckBox::init()
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

	sprite_chkmark = WidgetSpriteShader::create(get_gui(), "white.texture");
	get_parent_widget()->addChild(sprite_chkmark, Gui::ALIGN_OVERLAP);

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
			setChecked(checked.get() == 0);
	});
}

void CheckBox::shutdown()
{
	label.deleteLater();
	sprite_chkmark.deleteLater();
	sprite_w.deleteLater();
	mat.deleteLater();

	Element::shutdown();
}

void CheckBox::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setChecked(checked.get() != 0);

	setFont(font_file);
	setText(text);
	setFontPercent(font_percent);
	setFontOffset(font_offset);
	setFontColor(font_color);
	setFontOutline(font_outline.get() == 1);

	setBackgroundMaterial(bg_material);
	setBackgroundTexture(bg_texture_file);
	setBackgroundUV(bg_uv);
	setBackgroundColor(bg_color);

	setCheckmarkTexture(checkmark_texture_file);
	setCheckmarkPercent(checkmark_percent);

	if (unlock_arrange())
		arrange();
}

void CheckBox::setChecked(bool in_toggled)
{
	int prev_checked = checked.get();
	checked = in_toggled ? 1 : 0;
	sprite_chkmark->setHidden(!in_toggled);

	if (prev_checked != checked.get())
		changed_event.run(this);
}

void CheckBox::setText(const char *in_text)
{
	text = in_text;
	StringStack<> t = Localization::get(text);
	t = t.replace("\\n", "\n");
	label->setText(t);
	arrange();
}

void CheckBox::setFontPercent(float value)
{
	font_percent = value;
	arrange();
}

void CheckBox::setFontOffset(const Unigine::Math::vec2 &offset)
{
	font_offset = offset;
	arrange();
}

void CheckBox::setFontColor(const vec4 &color)
{
	font_color = color;
	label->setFontColor(font_color);
}

void CheckBox::setFontOutline(bool outline)
{
	font_outline = outline ? 1 : 0;
	label->setFontOutline(outline ? 1 : 0);
}

void CheckBox::setFont(const char *font)
{
	if (font_file_stored == font)
		return;

	font_file = font;
	font_file_stored = font;
	label->setFont(font);
}

void CheckBox::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
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

void CheckBox::setBackgroundTexture(const char *texture_path)
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

void CheckBox::setBackgroundTexture(const TexturePtr &texture)
{
	sprite_w->setRender(texture);
	sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	mat_vars.setTextureSize(sprite_size);
}

void CheckBox::setBackgroundUV(const vec4 &uv)
{
	bg_uv = uv;
	sprite_w->setLayerTexCoord(0, uv);
}

void CheckBox::setBackgroundUV(float u0, float v0, float u1, float v1)
{
	bg_uv = vec4(u0, v0, u1, v1);
	sprite_w->setLayerTexCoord(0, bg_uv);
}

void CheckBox::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	sprite_w->setColor(bg_color);
}

void CheckBox::setCheckmarkTexture(const char *texture_path)
{
	checkmark_texture_file = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(sprite_chkmark->getTexture(), texture_path) == 0)
		return;

	sprite_chkmark->setTexture(texture_path);
}

void CheckBox::setCheckmarkTexture(const TexturePtr &texture)
{
	sprite_chkmark->setRender(texture);
}

void CheckBox::setCheckmarkPercent(float value)
{
	checkmark_percent = value;
	arrange();
}

void CheckBox::click()
{
	if (active.get() == 0)
		return;

	if (isFocusable())
		setFocus(true);

	set_state(State::Press, true);
	apply_state_animation();

	// change checked state
	setChecked(checked.get() == 0);
}

void CheckBox::on_enable()
{
	Element::on_enable();
	if (sprite_w)
	{
		sprite_w->setHidden(false);
		sprite_chkmark->setHidden(checked.get() == 0);
		label->setHidden(false);
	}
}

void CheckBox::on_disable()
{
	if (sprite_w)
	{
		sprite_chkmark->setHidden(true);
		sprite_w->setHidden(true);
		label->setHidden(true);
	}
	Element::on_disable();
}

void CheckBox::update(float ifps)
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

void CheckBox::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	int s_x = get_screen_x();
	int s_y = get_screen_y();
	int s_w = max(1, get_screen_width());
	int s_h = max(1, get_screen_height());

	// arrange sprites
	sprite_w->setPosition(s_x, s_y);
	sprite_w->setWidth(s_h);
	sprite_w->setHeight(s_h);
	sprite_w->arrange();
	mat_vars.setSpritePosAndSize(sprite_w, canvas->getScreenSize());

	if (checkmark_percent.get() == 0)
	{
		sprite_chkmark->setPosition(s_x, s_y);
		sprite_chkmark->setWidth(s_h);
		sprite_chkmark->setHeight(s_h);
	}
	else
	{
		int o = ftoi((s_h * checkmark_percent.get() - s_h) / 2.0f);
		sprite_chkmark->setPosition(s_x - o, s_y - o);
		sprite_chkmark->setWidth(s_h + o * 2);
		sprite_chkmark->setHeight(s_h + o * 2);
	}
	sprite_chkmark->arrange();

	// arrange label
	float font_height = (max_n.y - min_n.y) * canvas->getCanvasHeight() * font_percent;
	label->setFontSize(canvas->getFontSize(font_height));
	label->setWidth(s_w);
	label->arrange();
	int x = s_x + s_h + ftoi(s_h * saturate(1.0f - font_percent));
	int y = s_y + s_h / 2 - label->getHeight() / 2;
	x += canvas->convertCanvasToScreen(font_offset.get().x);
	y += canvas->convertCanvasToScreen(font_offset.get().y);
	label->setPosition(x, y);
}

void CheckBox::apply_order_to_widgets()
{
	if (!sprite_w)
		return;
	sprite_w->setOrder(getOrder());
	sprite_chkmark->setOrder(getOrder());
	label->setOrder(getOrder());

	WidgetPtr parent = sprite_w->getParent();
	parent->removeChild(sprite_w);
	parent->addChild(sprite_w);
	parent->removeChild(sprite_chkmark);
	parent->addChild(sprite_chkmark);
	parent->removeChild(label);
	parent->addChild(label);
}

void CheckBox::set_gui(const Unigine::GuiPtr &gui)
{
	if (!sprite_w)
		return;
	if (gui != sprite_w->getParentGui())
	{
		gui->addChild(sprite_w);
		gui->addChild(sprite_chkmark);
		gui->addChild(label);
	}
}

void CheckBox::apply_state_animation()
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
		int w = max(1, get_screen_height());
		int h = w;
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
