#include "ClipMask.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(ClipMask);

void ClipMask::init_canvas()
{
	if (gui)
		return;

	gui = Gui::create();
	gui->setExposeSpeed(0);	   // instant show
}

void ClipMask::init()
{
	if (element_initialized)
		return;

	init_canvas();

	lock_arrange();
	Element::init();
	unlock_arrange();

	mask_sprite = WidgetSpriteShader::create(get_gui(), "white.texture");
	mask_sprite->arrange();
	mask_sprite_size = vec3(itof(mask_sprite->getWidth()), itof(mask_sprite->getHeight()), 0);
	get_parent_widget()->addChild(mask_sprite, Gui::ALIGN_OVERLAP);

	// apply parameters
	applyPropertyChanges();
	arrange();
	apply_order_to_widgets();
}

void ClipMask::shutdown()
{
	mask_sprite.deleteLater();
	mask_mat.deleteLater();
	gui.deleteLater();

	Element::shutdown();
}

void ClipMask::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setMaskMaterial(mask_material);
	setMaskTexture(mask_texture_file);
	setMaskUV(mask_uv);
	setMaskColor(mask_color);

	if (unlock_arrange())
		arrange();
}

void ClipMask::setMaskMaterial(const Unigine::MaterialPtr &in_material)
{
	if (!in_material && !mask_mat)
		return;

	if (mask_mat && mask_mat->getParent() == in_material)
		return;

	mask_mat.deleteLater();
	if (in_material)
	{
		mask_mat = in_material->inherit();

		mask_mat_vars.setMaterial(mask_mat);
		mask_mat_vars.setTextureSize(mask_sprite_size);
		mask_mat_vars.setSpritePosAndSize(mask_sprite, canvas->getScreenSize());
		mask_sprite->setMaterial(mask_mat);
	}
	else
	{
		mask_mat_vars.setMaterial(nullptr);
		mask_sprite->setMaterial(nullptr);
	}
}

void ClipMask::setMaskTexture(const char *texture_path)
{
	if (!mask_mat || mask_mat->getNumTextures() < 2)
		return;

	mask_texture_file = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (mask_mat && strcmp(mask_mat->getTexturePath(1), texture_path) == 0)
		return;

	mask_mat->setTexturePath(1, texture_path);

	// get image size
	ImagePtr img = Image::create();
	if (img->info(texture_path))
		mask_mat_vars.setTextureSize(vec3(itof(img->getWidth()), itof(img->getHeight()), 0));
}

void ClipMask::setMaskTexture(const TexturePtr &texture)
{
	mask_mat->setTexture(1, texture);
	mask_mat_vars.setTextureSize(vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0));
}

void ClipMask::setMaskUV(const Unigine::Math::vec4 &uv)
{
	mask_uv = uv;
	if (mask_mat)
		mask_mat->setParameterFloat4("mask_uv", uv);
}

void ClipMask::setMaskUV(float u0, float v0, float u1, float v1)
{
	mask_uv = vec4(u0, v0, u1, v1);
	if (mask_mat)
		mask_mat->setParameterFloat4("mask_uv", mask_uv);
}

void ClipMask::setMaskColor(const Unigine::Math::vec4 &in_color)
{
	mask_color = in_color;
	mask_sprite->setColor(in_color);
}

void ClipMask::addChild(Element *element)
{
	UNIGINE_ASSERT(element && this != element);

	if (children.findIndex(element) != -1)
		return;	   // added already

	// detach from previous parent
	if (element->parent)
		element->parent->removeChild(element);

	// attach to this
	children.append(element);
	element->parent = this;
	element->set_canvas_hierarchy(this);
	element->update_enabled_hierarchy();
	element->arrange_hierarchy();
	element->update_order_hierarchy();

	// notify canvas
	canvas->on_add_element(element);
}

int ClipMask::convertCanvasToScreen(float canvas_position) const
{
	return canvas->convertCanvasToScreen(canvas_position);
}

Unigine::Math::ivec2 ClipMask::convertCanvasToScreen(
	const Unigine::Math::vec2 &canvas_position) const
{
	return canvas->convertCanvasToScreen(canvas_position);
}

float ClipMask::convertScreenToCanvas(int screen_position) const
{
	return canvas->convertScreenToCanvas(screen_position);
}

Unigine::Math::vec2 ClipMask::convertScreenToCanvas(
	const Unigine::Math::ivec2 &screen_position) const
{
	return canvas->convertScreenToCanvas(screen_position);
}

void ClipMask::on_enable()
{
	Element::on_enable();
	if (mask_sprite)
		mask_sprite->setHidden(false);
}

void ClipMask::on_disable()
{
	if (mask_sprite)
		mask_sprite->setHidden(true);
	Element::on_disable();
}

void ClipMask::update(float ifps)
{
	int width = get_screen_width();
	int height = get_screen_height();
	if (width <= 0 || height <= 0)
		return;

	// update material
	mask_mat_vars.setMousePosition(mask_sprite, canvas->getMouseScreenPosition());
	mask_mat_vars.runExpressionUpdate(mask_sprite);

	gui->setSize(ivec2(width, height));
	gui->getVBox()->setPosition(-get_screen_x(), -get_screen_y());

	if (!render_texture)
	{
		render_target = RenderTarget::create();
		render_texture = Texture::create();
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		render_texture->clearBuffer(vec4_zero);
		mask_sprite->setRender(render_texture);
	}
	else if (width != render_texture->getWidth() || height != render_texture->getHeight())
	{
		render_texture->create2D(
			width, height, Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		render_texture->clearBuffer(vec4_zero);
		mask_sprite->setRender(render_texture);
	}

	RenderState::saveState();

	render_texture->clearBuffer(vec4_zero);
	render_target->bindColorTexture(0, render_texture);
	render_target->enable();
	{
		gui->enable();
		gui->update();
		gui->preRender();
		gui->render();
		gui->disable();
	}
	render_target->disable();
	render_target->unbindAll();

	RenderState::restoreState();
}

void ClipMask::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	mask_sprite->setPosition(get_screen_x(), get_screen_y());
	mask_sprite->setWidth(get_screen_width());
	mask_sprite->setHeight(get_screen_height());
	mask_sprite->arrange();
	mask_mat_vars.setSpritePosAndSize(mask_sprite, canvas->getScreenSize());
}

void ClipMask::apply_order_to_widgets()
{
	if (!mask_sprite)
		return;
	mask_sprite->setOrder(getOrder());
	WidgetPtr parent = mask_sprite->getParent();
	parent->removeChild(mask_sprite);
	parent->addChild(mask_sprite);
}

void ClipMask::set_canvas_hierarchy(UI::ICanvas *in_canvas)
{
	if (canvas == in_canvas)
		return;

	canvas = in_canvas;
	set_gui(canvas ? canvas->getGui() : Gui::getCurrent());

	for (auto &child : children)
		child->set_canvas_hierarchy(this);
}

void ClipMask::set_gui(const Unigine::GuiPtr &gui)
{
	if (!mask_sprite)
		return;
	if (gui != mask_sprite->getParentGui())
		gui->addChild(mask_sprite);
}
