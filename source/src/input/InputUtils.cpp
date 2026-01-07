#include "InputUtils.h"

using namespace Unigine;
using namespace Unigine::Math;

bool isPressed(Input::KEY a)
{
	return Input::isKeyDown(a);
}

bool isDown(Input::KEY a)
{
	return Input::isKeyPressed(a);
}

bool isUp(Input::KEY a)
{
	return Input::isKeyUp(a);
}

float getAxis(Input::KEY a, Input::KEY b)
{
	return Input::isKeyPressed(a) - Input::isKeyPressed(b);
}

Math::vec2 getAxis(Input::KEY a1, Input::KEY b1, Input::KEY a2, Input::KEY b2)
{
	return {getAxis(a1, b1), getAxis(a2, b2)};
}
