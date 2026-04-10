#include "WorldSpaceUIController.h"

#include "../../ui/elements/Canvas.h"

#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineVisualizer.h>

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(WorldSpaceUIController);

void WorldSpaceUIController::init()
{
	if (canvas_node.get())
	{
		auto component = getComponent<UI::Canvas>(canvas_node.get());
		if (component)
			canvas = component->getPtr();
		else
			Log::error(
				"WorldSpaceUIController::init(): Node \"%s\" doesn't have \"Canvas\" component\n",
				canvas_node.get()->getName());
	}

	if (cursor_element_node.get())
	{
		auto component = getComponent<UI::Element>(cursor_element_node.get());
		if (component)
			cursor_element = component->getPtr();
	}

	obj = checked_ptr_cast<Object>(node);
	if (!obj)
		Log::error(
			"WorldSpaceUIController::init(): Node \"%s\" is not an Object!\n", node->getName());
}

void WorldSpaceUIController::post_update()
{
	// update mouse cursor position
	vec2 mouse_pos;
	bool visible = get_mouse_coords(mouse_pos);
	if (cursor_element)
	{
		cursor_element->setEnabled(visible);
		cursor_element->setPosition(mouse_pos);
	}

	// update mouse input
	int mouse_button = 0;
	if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT))
		mouse_button = 1;
	if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_MIDDLE))
		mouse_button |= (1 << 1);
	if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT))
		mouse_button |= (1 << 2);
	int mouse_scroll = Input::getMouseWheel();

	// update display
	if (canvas)
	{
		if (visible)
		{
			canvas->updateManual(
				mouse_pos, mouse_button, mouse_scroll, ivec2_zero, false, Game::getIFps());
		}
		else
		{
			canvas->updateManual(Game::getIFps());
		}
	}
}

bool WorldSpaceUIController::get_mouse_coords(Unigine::Math::vec2 &out_pos)
{
	PlayerPtr player = Game::getPlayer();
	if (!canvas || !obj || !player)
		return false;

	// check distance
	float max_distance_2 = pow2(max_distance.get());
	CameraPtr camera = player->getCamera();
	Vec3 cam_pos = camera->getPosition();
	if (length2(cam_pos - obj->getWorldPosition()) > max_distance_2)
		return false;

	// check intersection (camera's eyes vs display's surface)
	vec3 dir = vec3(normalize(-camera->getIModelview().getColumn3(2)));
	if (!Input::isMouseCursorHide() && WindowManager::getMainWindow())
	{
		// convert 2d mouse coords to 3d view direction (if needed)
		Math::ivec2 winsize = WindowManager::getMainWindow()->getClientSize();
		float width = itof(winsize.x);
		float height = itof(winsize.y);
		GuiPtr gui = Gui::getCurrent();
		dir = camera->getDirectionFromScreen(
			gui->getMouseX() / width, gui->getMouseY() / height, height / width);
	}
	Vec3 p0 = cam_pos;
	Vec3 p1 = p0 + Vec3(dir * camera->getZFar());
	Mat4 t = obj->getIWorldTransform();	   // world to local matrix
	if (!obj->getIntersection(t * p0, t * p1, intersection, 0))
		return false;

	// check that we see front face of the display surface
	t = obj->getWorldTransform();
	if (dot(t.getRotate() * intersection->getNormal(), dir) > 0)
		return false;

	// get coords
	vec4 uv = intersection->getTexCoord();
	out_pos = vec2(uv.x * canvas->getCanvasWidth(), uv.y * canvas->getCanvasHeight());
	return true;
}
