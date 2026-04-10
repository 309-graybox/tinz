#include "MouseHandleSetter.h"

#include "../../ui/elements/Canvas.h"

#include <UnigineInput.h>

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(MouseHandleSetter);

void MouseHandleSetter::init()
{
	if (mode.get() == 0)
	{
		Input::setMouseHandle(Input::MOUSE_HANDLE_GRAB);

		if (grab_at_start.get() == 1)
		{
			Input::setMouseGrab(true);
			Input::setMouseCursorHide(true);

			ControlsApp::setMouseHandle(Input::MOUSE_HANDLE_GRAB);
			ControlsApp::setMouseEnabled(true);
			ControlsApp::setEnabled(true);
		}
	}
	else if (mode.get() == 1)
		Input::setMouseHandle(Input::MOUSE_HANDLE_SOFT);
	else
		Input::setMouseHandle(Input::MOUSE_HANDLE_USER);
}
