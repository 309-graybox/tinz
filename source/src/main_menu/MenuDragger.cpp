#include "MenuDragger.h"

#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineLog.h>
#include <UnigineMathLibCommon.h>

REGISTER_COMPONENT(MenuDragger);

using namespace Unigine;
using namespace Math;

void MenuDragger::onInit()
{
	_rest_pos = buryNode->getWorldPosition();
	_value = clamp((float)initialValue, 0.0f, 1.0f);
	apply_value(_value);
	onValueChanged(_value);

	auto load_cursor = [this](const char *path, const char *kind) -> ImagePtr {
		if (!path || !*path)
			return ImagePtr();
		ImagePtr img = Image::create();
		if (!img->load(path))
		{
			Log::warning("MenuDragger on '%s': failed to load %s cursor '%s'\n",
				node ? node->getName() : "<null>", kind, path);
			return ImagePtr();
		}
		return img;
	};

	_hover_cursor_image = load_cursor(hoverCursor.get(), "hover");
	_drag_cursor_image = load_cursor(dragCursor.get(), "drag");
}

void MenuDragger::onUpdate()
{
	if (!node)
		return;

	const Vec3 target = _rest_pos + Vec3(buryOffset.get()) * (1.0f - _value);
	const Vec3 cur = buryNode->getWorldPosition();
	const float t = saturate((float)easeSpeed * Game::getIFps());
	buryNode->setWorldPosition(lerp(cur, target, t));
}

void MenuDragger::setHovered(bool on, bool play_sound)
{
	MenuInteractive::setHovered(on, play_sound);
	apply_cursor();
}

void MenuDragger::beginDrag(int mouse_y)
{
	_dragging = true;
	_drag_start_value = _value;
	_drag_start_mouse_y = mouse_y;
	apply_cursor();
}

void MenuDragger::updateDrag(int mouse_y)
{
	if (!_dragging)
		return;

	// Screen Y grows downward → invert so dragging up raises value.
	const float dy = (float)(_drag_start_mouse_y - mouse_y);
	const float sens = max((float)sensitivity, 1.0f);
	const float new_value = clamp(_drag_start_value + dy / sens, 0.0f, 1.0f);
	if (new_value != _value)
	{
		_value = new_value;
		onValueChanged(_value);
	}
}

void MenuDragger::endDrag()
{
	_dragging = false;
	apply_cursor();
}

void MenuDragger::apply_value(float v01)
{
	if (!node)
		return;
	const Vec3 target = _rest_pos + Vec3(buryOffset.get()) * (1.0f - v01);
	buryNode->setWorldPosition(target);
}

void MenuDragger::apply_cursor()
{
	ImagePtr img;
	if (_dragging)
		img = _drag_cursor_image ? _drag_cursor_image : _hover_cursor_image;
	else if (isHovered())
		img = _hover_cursor_image;

	if (img)
		Input::setMouseCursorCustom(img, img->getWidth() / 2, img->getHeight() / 2);
	else
		Input::clearMouseCursorCustom();
}
