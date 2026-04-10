// Copyright (C), UNIGINE. All rights reserved.

#pragma once
#include "imgui/imgui.h"

#include <UnigineWidgets.h>

class ImGuiImpl
{
public:
	static void init(const Unigine::WidgetPtr &root_widget);
	static void newFrame();
	static void render();
	static void shutdown();

	// info
	static bool isWantCaptureMouse();
	static bool isWantCaptureKeyboard();

	// ui
	static Unigine::WidgetSpritePtr &getWidget();
	static void setBackgroundColor(const Unigine::Math::vec4 &color);
	static void bringToFront();

private:
	static Unigine::EventConnections event_connections;

	static float last_scale;
};
