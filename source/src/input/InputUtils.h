#pragma once
#include <UnigineInput.h>

bool isPressed(Unigine::Input::KEY a);
bool isDown(Unigine::Input::KEY a);
bool isUp(Unigine::Input::KEY a);

float getAxis(Unigine::Input::KEY a, Unigine::Input::KEY b);
Unigine::Math::vec2 getAxis(Unigine::Input::KEY a1, Unigine::Input::KEY b1, Unigine::Input::KEY a2, Unigine::Input::KEY b2);
