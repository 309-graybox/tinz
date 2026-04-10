#include "ScrollBox.h"

#include "../../localization/Localization.h"
#include "Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(ScrollBox);

void ScrollBox::init()
{
	if (element_initialized)
		return;

	lock_arrange();
	Element::init();
	unlock_arrange();

	// create scroll box background
	w_sprite = WidgetSpriteShader::create(get_gui(), "white.texture");
	w_sprite->arrange();
	w_sprite_size = vec3(itof(w_sprite->getWidth()), itof(w_sprite->getHeight()), 0);
	get_parent_widget()->addChild(w_sprite, Gui::ALIGN_OVERLAP);

	// create viewport (clipmask)
	if (viewport_node)
	{
		auto clip_mask = getComponent<ClipMask>(viewport_node);
		if (clip_mask)
			viewport = clip_mask->getPtr();
	}
	if (!viewport)
	{
		// node: we can't use addComponent<> inside init() methods
		// we need to call it later next frame
		auto *one_shot = new Unigine::EventConnection;
		Unigine::Engine::get()->getEventBeginWorldUpdate().connect(*one_shot, [&, one_shot,
																				  this]() {
			NodeDummyPtr c_node = NodeDummy::create();
			c_node->setShowInEditorEnabled(true);
			c_node->setSaveToWorldEnabled(true);
			c_node->setParent(getNode());
			c_node->setName("viewport");
			viewport = ComponentSystem::get()->addComponent<ClipMask>(c_node)->getPtr();
			viewport->mask_material = Materials::findManualMaterial("ui_clipmask_nine_sliced");
			viewport->mask_texture_file = GET_GUID("UnigineToolkit/ui/textures/scrollbox_mask.png");
			viewport->anchor = vec4(0, 0, 1, 1);
			viewport->pos = vec4(0, 0, 32, 32);
			viewport_node = c_node;
			subscribe_to_viewport();
			delete one_shot;
		});
	}
	else
		subscribe_to_viewport();

	// create content (element)
	if (content_node)
	{
		auto element = getComponent<Element>(content_node);
		if (element)
			content = element->getPtr();
	}
	if (!content)
	{
		auto *one_shot = new Unigine::EventConnection;
		Unigine::Engine::get()->getEventBeginWorldUpdate().connect(
			*one_shot, [&, one_shot, this]() {
				NodeDummyPtr c_node = NodeDummy::create();
				c_node->setShowInEditorEnabled(true);
				c_node->setSaveToWorldEnabled(true);
				c_node->setParent(viewport->getNode());
				c_node->setName("content");
				content = ComponentSystem::get()->addComponent<Element>(c_node)->getPtr();
				content->anchor = vec4(0, 0, 0, 0);
				content->pos = vec4(0, 0, 512, 512);
				content->setInteractable(false);	// to allow dragging this scroll box
				content_node = c_node;
				delete one_shot;
			});
	}

	// create horizontal scroll (scroll)
	if (horizontal_scroll_node)
	{
		auto scroll = getComponent<Scroll>(horizontal_scroll_node);
		if (scroll)
			horizontal_scroll = scroll->getPtr();
	}
	if (!horizontal_scroll)
	{
		auto *one_shot = new Unigine::EventConnection;
		Unigine::Engine::get()->getEventBeginWorldUpdate().connect(
			*one_shot, [&, one_shot, this]() {
				NodeDummyPtr c_node = NodeDummy::create();
				c_node->setShowInEditorEnabled(true);
				c_node->setSaveToWorldEnabled(true);
				c_node->setParent(getNode());
				c_node->setName("horizontal scroll");
				horizontal_scroll = ComponentSystem::get()->addComponent<Scroll>(c_node)->getPtr();
				horizontal_scroll->anchor = vec4(0, 1, 1, 1);
				horizontal_scroll->pivot = vec2(0, 1);
				horizontal_scroll->pos = vec4(0, 0, 32, 32);
				horizontal_scroll_node = c_node;
				subscribe_to_horizontal_scroll();
				delete one_shot;
			});
	}
	else
		subscribe_to_horizontal_scroll();

	// create vertical scroll (scroll)
	if (vertical_scroll_node)
	{
		auto scroll = getComponent<Scroll>(vertical_scroll_node);
		if (scroll)
			vertical_scroll = scroll->getPtr();
	}
	if (!vertical_scroll)
	{
		auto *one_shot = new Unigine::EventConnection;
		Unigine::Engine::get()->getEventBeginWorldUpdate().connect(
			*one_shot, [&, one_shot, this]() {
				NodeDummyPtr c_node = NodeDummy::create();
				c_node->setShowInEditorEnabled(true);
				c_node->setSaveToWorldEnabled(true);
				c_node->setParent(getNode());
				c_node->setName("vertical scroll");
				vertical_scroll = ComponentSystem::get()->addComponent<Scroll>(c_node)->getPtr();
				vertical_scroll->anchor = vec4(1, 0, 1, 1);
				vertical_scroll->pivot = vec2(1, 0);
				vertical_scroll->pos = vec4(0, 0, 32, 32);
				vertical_scroll->setDirection(UI::Scroll::DIRECTION::TOP_TO_BOTTOM);
				vertical_scroll_node = c_node;
				subscribe_to_vertical_scroll();
				delete one_shot;
			});
	}
	else
		subscribe_to_vertical_scroll();

	// apply parameters
	applyPropertyChanges();

	// sync with hierarchy (applyPropertyChanges() doesn't call arrange() inside if
	// all parameters are default, but we need to call it at least once here)
	arrange();
	apply_order_to_widgets();
}

void ScrollBox::shutdown()
{
	w_sprite.deleteLater();
	w_mat.deleteLater();

	Element::shutdown();
}

void ScrollBox::applyPropertyChanges()
{
	Element::applyPropertyChanges();
	lock_arrange();

	setHorizontalScrollMode(static_cast<SCROLL_MODE>(horizontal_scroll_mode.get()));
	setVerticalScrollMode(static_cast<SCROLL_MODE>(vertical_scroll_mode.get()));

	setBackgroundMaterial(bg_material);
	setBackgroundTexture(bg_texture_param);
	setBackgroundUV(bg_uv);
	setBackgroundColor(bg_color);

	if (unlock_arrange())
		arrange();
}

void ScrollBox::setHorizontalScrollMode(SCROLL_MODE mode)
{
	horizontal_scroll_mode = static_cast<int>(mode);
	arrange();
}

ScrollBox::SCROLL_MODE ScrollBox::getHorizontalScrollMode() const
{
	return static_cast<ScrollBox::SCROLL_MODE>(horizontal_scroll_mode.get());
}

void ScrollBox::setVerticalScrollMode(SCROLL_MODE mode)
{
	vertical_scroll_mode = static_cast<int>(mode);
	arrange();
}

ScrollBox::SCROLL_MODE ScrollBox::getVerticalScrollMode() const
{
	return static_cast<ScrollBox::SCROLL_MODE>(vertical_scroll_mode.get());
}

void ScrollBox::setBackgroundMaterial(const Unigine::MaterialPtr &in_material)
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

void ScrollBox::setBackgroundTexture(const char *texture_path)
{
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

void ScrollBox::setBackgroundTexture(const TexturePtr &texture)
{
	w_sprite->setRender(texture);
	w_sprite_size = vec3(itof(texture->getWidth()), itof(texture->getHeight()), 0);
	arrange();	  // apply rotation and fixed_ratio

	w_mat_vars.setTextureSize(w_sprite_size);
}

void ScrollBox::setBackgroundUV(const vec4 &uv)
{
	bg_uv = uv;
	w_sprite->setLayerTexCoord(0, uv);
}

void ScrollBox::setBackgroundUV(float u0, float v0, float u1, float v1)
{
	bg_uv = vec4(u0, v0, u1, v1);
	w_sprite->setLayerTexCoord(0, bg_uv);
}

void ScrollBox::setBackgroundColor(const Unigine::Math::vec4 &color)
{
	bg_color = color;
	w_sprite->setColor(bg_color);
}

void ScrollBox::on_enable()
{
	Element::on_enable();
	if (w_sprite)
		w_sprite->setHidden(false);
}

void ScrollBox::on_disable()
{
	if (w_sprite)
		w_sprite->setHidden(true);
	Element::on_disable();
}

void ScrollBox::update(float ifps)
{
	// update material
	w_mat_vars.setMousePosition(w_sprite, canvas->getMouseScreenPosition());
	w_mat_vars.runExpressionUpdate(w_sprite);

	vec2 viewport_size = viewport ? viewport->getSize() : vec2_zero;
	vec2 content_size = content ? content->getSize() : vec2_zero;

	// moving
	if (moving && !canvas->isMouseButtonLeftPressed())
		moving = false;
	if (Engine::get()->isEditorLoaded())
		moving = false;
	if (moving && content)
	{
		vec2 mouse_pos = canvas->getMouseCanvasPosition();
		vec2 new_pos = content->getPositionLeftTop() + mouse_pos - mouse_start_pos;
		new_pos = clamp(new_pos, -content_size + viewport_size, vec2(0, 0));
		content->setPositionLeftTop(new_pos);
		mouse_start_pos = mouse_pos;
	}

	// set value to scrolls
	if (viewport && content)
	{
		if (horizontal_scroll)
		{
			float scroll_size = content_size.x != 0 ? (viewport_size.x / content_size.x) : 1.0f;
			horizontal_scroll->setScrollSize(scroll_size);
			float value = -content->getPositionLeftTop().x / (content_size.x - viewport_size.x);
			horizontal_scroll->setValue(value);
		}
		if (vertical_scroll)
		{
			float scroll_size = content_size.y != 0 ? (viewport_size.y / content_size.y) : 1.0f;
			vertical_scroll->setScrollSize(scroll_size);
			float value = -content->getPositionLeftTop().y / (content_size.y - viewport_size.y);
			vertical_scroll->setValue(value);
		}
	}
}

void ScrollBox::arrange()
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

	if (viewport)
	{
		SCROLL_MODE h_mode = getHorizontalScrollMode();
		SCROLL_MODE v_mode = getVerticalScrollMode();
		float h_offset = vertical_scroll ? vertical_scroll->getSize().x : 0;
		float v_offset = horizontal_scroll ? horizontal_scroll->getSize().y : 0;
		if (h_mode == SCROLL_MODE::NONE)
		{
			v_offset = 0;
			if (horizontal_scroll)
				horizontal_scroll->setEnabled(false);
		}
		if (v_mode == SCROLL_MODE::NONE)
		{
			h_offset = 0;
			if (vertical_scroll)
				vertical_scroll->setEnabled(false);
		}
		viewport->anchor = vec4(0, 0, 1, 1);
		viewport->pos = vec4(0, 0, h_offset, v_offset);

		vec2 viewport_size = viewport->getSize();
		vec2 content_size = content ? content->getSize() : vec2_zero;

		if (h_mode == SCROLL_MODE::AUTO_HIDE)
		{
			bool show = true;
			if (viewport_size.x >= content_size.x)
			{
				v_offset = 0;
				show = false;
			}
			if (horizontal_scroll)
				horizontal_scroll->setEnabled(show);
		}
		else if (h_mode == SCROLL_MODE::ALWAYS_VISIBLE)
		{
			if (horizontal_scroll)
				horizontal_scroll->setEnabled(true);
		}
		if (v_mode == SCROLL_MODE::AUTO_HIDE)
		{
			bool show = true;
			if (viewport_size.y >= content_size.y)
			{
				h_offset = 0;
				show = false;
			}
			if (vertical_scroll)
				vertical_scroll->setEnabled(show);
		}
		else if (v_mode == SCROLL_MODE::ALWAYS_VISIBLE)
		{
			if (vertical_scroll)
				vertical_scroll->setEnabled(true);
		}
		viewport->pos = vec4(0, 0, h_offset, v_offset);
		if (horizontal_scroll)
			horizontal_scroll->setRightOffset(h_offset);
		if (vertical_scroll)
			vertical_scroll->setBottomOffset(v_offset);
	}
}

void ScrollBox::apply_order_to_widgets()
{
	if (!w_sprite)
		return;
	w_sprite->setOrder(getOrder());
	WidgetPtr parent = w_sprite->getParent();
	parent->removeChild(w_sprite);
	parent->addChild(w_sprite);
}

void ScrollBox::set_gui(const Unigine::GuiPtr &gui)
{
	if (!w_sprite)
		return;
	if (gui != w_sprite->getParentGui())
		gui->addChild(w_sprite);
}

void ScrollBox::subscribe_to_viewport()
{
	viewport->getEventMouseClickEnter().connect(*this, [this](Element *) {
		moving = true;
		mouse_start_pos = canvas->getMouseCanvasPosition();
	});
	viewport->getEventMouseClickExit().connect(*this, [this](Element *) { moving = false; });
}

void ScrollBox::subscribe_to_horizontal_scroll()
{
	horizontal_scroll->getEventScrollChanged().connect(*this, [this](Scroll *) {
		float value = horizontal_scroll->getValue();
		vec2 viewport_size = viewport ? viewport->getSize() : vec2_zero;
		vec2 content_size = content ? content->getSize() : vec2_zero;
		vec2 pos = content->getPositionLeftTop();
		pos.x = (viewport_size.x - content_size.x) * value;
		content->setPositionLeftTop(pos);
	});
}

void ScrollBox::subscribe_to_vertical_scroll()
{
	vertical_scroll->getEventScrollChanged().connect(*this, [this](Scroll *) {
		float value = vertical_scroll->getValue();
		vec2 viewport_size = viewport ? viewport->getSize() : vec2_zero;
		vec2 content_size = content ? content->getSize() : vec2_zero;
		vec2 pos = content->getPositionLeftTop();
		pos.y = (viewport_size.y - content_size.y) * value;
		content->setPositionLeftTop(pos);
	});
}
