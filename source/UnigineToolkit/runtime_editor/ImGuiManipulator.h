// Copyright (C), UNIGINE. All rights reserved.
#pragma once
#include <UnigineMathLib.h>
#include <UnigineWidgets.h>

class ImGuiManipulator
{
public:
	static void begin();
	static bool show(Unigine::Math::Mat4 &transform,
		int type = Unigine::Widget::WIDGET_MANIPULATOR_TRANSLATOR,
		const Unigine::Math::Mat4 &basis = Unigine::Math::Mat4_identity);
	static void end();
	static bool isHovered();
	static void destroy();
};
