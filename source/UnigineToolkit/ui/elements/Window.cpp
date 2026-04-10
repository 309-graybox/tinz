#include "Window.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(Window);

void Window::init()
{
	if (element_initialized)
		return;

	lock_arrange();
	Element::init();
	unlock_arrange();

	// create window
	w_sprite = WidgetSpriteShader::create(get_gui(), "white.texture");
	w_sprite->arrange();
	w_sprite_size = vec3(itof(w_sprite->getWidth()), itof(w_sprite->getHeight()), 0);
	get_parent_widget()->addChild(w_sprite, Gui::ALIGN_OVERLAP);

	// create content (clipmask)
	if (content_node)
	{
		auto clip_mask = getComponent<ClipMask>(content_node);
		if (clip_mask)
			content = clip_mask->getPtr();
	}
	if (!content)
	{
		// node: we can't use addComponent<> inside init() methods
		// we need to call it later next frame
		auto *one_shot = new Unigine::EventConnection;
		Unigine::Engine::get()->getEventBeginWorldUpdate().connect(
			*one_shot, [&, one_shot, this]() {
				NodeDummyPtr c_node = NodeDummy::create();
				c_node->setShowInEditorEnabled(true);
				c_node->setSaveToWorldEnabled(true);
				c_node->setParent(getNode());
				c_node->setName("content");
				content = ComponentSystem::get()->addComponent<ClipMask>(c_node)->getPtr();
				content->anchor = vec4(0, 0, 1, 1);
				content->pos = vec4(0, 0, 0, 0);
				content->setInteractable(false);	// to allow dragging this window
				content_node = c_node;
				delete one_shot;
			});
	}

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();

	getEventMouseClickEnter().connect(*this, [this](Element *) {
		const int handle_size = 16;
		ivec2 p = canvas->getMouseScreenPosition();
		if (isSizeable() && p.x > w_sprite->getPositionX() + w_sprite->getWidth() - handle_size
			&& p.y > w_sprite->getPositionY() + w_sprite->getHeight() - handle_size)
		{
			resizing = true;
		}
		else
			moving = true;

		mouse_start_pos = canvas->getMouseCanvasPosition();
	});
	getEventMouseClickExit().connect(*this, [this](Element *) {
		moving = false;
		resizing = false;
	});
}

void Window::shutdown()
{
	w_sprite.deleteLater();
	w_mat.deleteLater();

	Element::shutdown();
}

void Window::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	// setMoveable(moveable); // optimization: doesn't needed here
	// setSizeable(sizeable);
	setBackgroundMaterial(bg_material);
	setBackgroundTexture(isSizeable() ? bg_texture_sizeable_param : bg_texture_param);
	setBackgroundUV(bg_uv);
	setBackgroundColor(bg_color);

	if (unlock_arrange())
		arrange();
}

void Window::setSizeable(bool value)
{
	if (isSizeable() == value)
		return;

	sizeable = value ? 1 : 0;
	setBackgroundTexture(isSizeable() ? bg_texture_sizeable_param : bg_texture_param);
}

void Window::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
{
	if (!in_material && !w_mat)
		return;

	if (w_mat && w_mat->getParent() == in_material)
		return;

	w_mat.deleteLater();
	if (in_material)
	{
		w_mat = in_material->inherit();

		w_mat_vars.setMaterial(w_mat);
		w_mat_vars.setTextureSize(w_sprite_size);
		w_mat_vars.setSpritePosAndSize(w_sprite, canvas->getScreenSize());
		w_sprite->setMaterial(w_mat);
	}
	else
	{
		w_mat_vars.setMaterial(nullptr);
		w_sprite->setMaterial(nullptr);
	}
}

void Window::setBackgroundTexture(const char *texture_path)
{
	if (isSizeable())
		bg_texture_sizeable_param = texture_path;
	else
		bg_texture_param = texture_path;

	texture_path = Localization::get(texture_path);
	if (!texture_path || texture_path[0] == '\0')
		texture_path = "core/textures/common/white.texture";

	if (strcmp(w_sprite->getTexture(), texture_path) == 0)
		return;

	w_sprite->setTransform(mat4_identity);
	w_sprite->setWidth(0);
	w_sprite->setHeight(0);
	w_sprite->setTexture(texture_path);
	w_sprite->arrange();
	w_sprite_size = vec3(itof(w_sprite->getWidth()), itof(w_sprite->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	w_mat_vars.setTextureSize(w_sprite_size);
}

void Window::setBackgroundTexture(const TexturePtr &texture)
{
	w_sprite->setRender(texture);
	w_sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	w_mat_vars.setTextureSize(w_sprite_size);
}

const char *Window::getBackgroundTexture() const
{
	return isSizeable() ? bg_texture_sizeable_param.get() : bg_texture_param.get();
}

void Window::setBackgroundUV(const vec4 &uv)
{
	bg_uv = uv;
	w_sprite->setLayerTexCoord(0, uv);
}

void Window::setBackgroundUV(float u0, float v0, float u1, float v1)
{
	bg_uv = vec4(u0, v0, u1, v1);
	w_sprite->setLayerTexCoord(0, bg_uv);
}

void Window::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	w_sprite->setColor(bg_color);
}

void Window::on_enable()
{
	Element::on_enable();
	if (w_sprite)
		w_sprite->setHidden(false);
}

void Window::on_disable()
{
	if (w_sprite)
		w_sprite->setHidden(true);
	Element::on_disable();
}

void Window::update(float ifps)
{
	// update material
	w_mat_vars.setMousePosition(w_sprite, canvas->getMouseScreenPosition());
	w_mat_vars.runExpressionUpdate(w_sprite);

	// moving
	if (moving && !canvas->isMouseButtonLeftPressed())
		moving = false;
	if (Engine::get()->isEditorLoaded())
		moving = false;
	if (moving && isMoveable())
	{
		vec2 mouse_pos = canvas->getMouseCanvasPosition();
		setPositionLeftTop(getPositionLeftTop() + mouse_pos - mouse_start_pos);
		mouse_start_pos = mouse_pos;
	}

	// resizing
	if (resizing && !canvas->isMouseButtonLeftPressed())
		resizing = false;
	if (Engine::get()->isEditorLoaded())
		resizing = false;
	if (resizing && isSizeable())
	{
		vec2 mouse_pos = canvas->getMouseCanvasPosition();
		setSize(getSize() + mouse_pos - mouse_start_pos);
		mouse_start_pos = mouse_pos;
	}
}

void Window::arrange()
{
	if (is_arrange_locked())
		return;
	if (!canvas)
		return;

	w_sprite->setPosition(get_screen_x(), get_screen_y());
	w_sprite->setWidth(max(1, get_screen_width()));
	w_sprite->setHeight(max(1, get_screen_height()));
	w_sprite->arrange();
	w_mat_vars.setSpritePosAndSize(w_sprite, canvas->getScreenSize());

	if (content)
	{
		content->anchor = vec4(0, 0, 1, 1);
		content->pos = vec4(0, 0, 0, 0);
		content->setInteractable(false);	// to allow dragging this window
	}
}

void Window::apply_order_to_widgets()
{
	if (!w_sprite)
		return;
	w_sprite->setOrder(getOrder());
	WidgetPtr parent = w_sprite->getParent();
	parent->removeChild(w_sprite);
	parent->addChild(w_sprite);
}

void Window::set_gui(const Unigine::GuiPtr &gui)
{
	if (!w_sprite)
		return;
	if (gui != w_sprite->getParentGui())
		gui->addChild(w_sprite);
}
