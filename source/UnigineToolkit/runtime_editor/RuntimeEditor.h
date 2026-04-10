// Copyright (C), UNIGINE. All rights reserved.
#pragma once
#include "EditorPlayer.h"
#include "RuntimeEditorExtension.h"

#include <UnigineInput.h>
#include <UnigineInterface.h>
#include <UniginePlayers.h>
#include <UnigineVisualizer.h>

class RuntimeEditor
{
public:
	static void init();
	static void update();
	static void shutdown();

	static void setHotkeyEnabled(Unigine::Input::KEY hotkey);
	static Unigine::Input::KEY getHotkeyEnabled();

	static void setEnabled(bool enable);
	static bool isEnabled();

	static bool isMouseAvailable();
	static bool isKeyboardAvailable();

	// selections
	static void clearNodeSelection();
	static void selectNode(const Unigine::NodePtr &node);
	static void selectNodes(const Unigine::Vector<Unigine::NodePtr> &nodes);
	static bool isSelectedNode();
	static Unigine::NodePtr getSelectedNode();
	static void getSelectedNodes(Unigine::Vector<Unigine::NodePtr> &nodes);

	// visualizer
	static bool isAllowToUseVisualizer(const char *group);
	static void setAllowToUseVisualizer(const char *group, bool value);
	static void renderBox(const char *group, const Unigine::Math::vec3 &size,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.0f, bool depth_test = true);
	static void renderPoint3D(const char *group, const Unigine::Math::Vec3 &v, float size,
		const Unigine::Math::vec4 &color, bool screen_space = false, float duration = 0.0f,
		bool depth_test = true);
	static void renderTriangle3D(const char *group, const Unigine::Math::Vec3 &v0,
		const Unigine::Math::Vec3 &v1, const Unigine::Math::Vec3 &v2,
		const Unigine::Math::vec4 &color, float duration = 0.0f, bool depth_test = true);
	static void renderLine3D(const char *group, const Unigine::Math::Vec3 &v0,
		const Unigine::Math::Vec3 &v1, const Unigine::Math::vec4 &color, float duration = 0.0f,
		bool depth_test = true);
	static void renderLine2D(const char *group, const Unigine::Math::vec2 &v0,
		const Unigine::Math::vec2 &v1, const Unigine::Math::vec4 &color, float order = 0.f,
		float duration = 0.f);
	static void renderVector(const char *group, const Unigine::Math::Vec3 &position_start,
		const Unigine::Math::Vec3 &position_end, const Unigine::Math::vec4 &color,
		float arrow_size = 0.25f, bool screen_space = false, float duration = 0.0f,
		bool depth_test = true);
	static void renderCircle(const char *group, float radius, const Unigine::Math::Mat4 &transform,
		const Unigine::Math::vec4 &color, float duration = 0.0f, bool depth_test = true);
	static void renderSphere(const char *group, float radius, const Unigine::Math::Mat4 &transform,
		const Unigine::Math::vec4 &color, float duration = 0.0f, bool depth_test = true);
	static void renderCylinder(const char *group, float radius, float height,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.0f, bool depth_test = true);
	static void renderCapsule(const char *group, float radius, float height,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.0f, bool depth_test = true);
	static void renderSolidBox(const char *group, const Unigine::Math::vec3 &size,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.0f, bool depth_test = true);
	static void renderSolidSphere(const char *group, float radius,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.0f, bool depth_test = true);
	static void renderSolidCylinder(const char *group, float radius, float height,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
		bool depth_test);
	static void renderSolidCapsule(const char *group, float radius, float height,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
		bool depth_test);
	static void renderMessage2D(const char *group, const Unigine::Math::vec3 &position,
		const Unigine::Math::vec3 &center, const char *str, const Unigine::Math::vec4 &color,
		int outline = 0, int font_size = -1, float duration = 0.0f);
	static void renderMessage3D(const char *group, const Unigine::Math::Vec3 &position,
		const Unigine::Math::vec3 &center, const char *str, const Unigine::Math::vec4 &color,
		int outline = 0, int font_size = -1, float duration = 0.0f);
	static void renderBoundBox(const char *group, const Unigine::Math::BoundBox &box,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.f, bool depth_test = true);
	static void renderBoundSphere(const char *group, const Unigine::Math::BoundSphere &sphere,
		const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color,
		float duration = 0.f, bool depth_test = true);

	// debug util
	static void focusToNode(const Unigine::NodePtr &node, bool attach = false);
	static void setTimeSpeed(float speed);
	static void setCameraTransform(const Unigine::Math::Mat4 &transform);

	// timer util
	static int addUiTimer();
	static void clearUiTimeout(int timer_id);
	static void setUiTimeout(int timer_id, double duration);
	static bool checkUiTimeout(int timer_id);

	// extensions
	static void addExtension(RuntimeEditorExtension *extension);
	static void removeExtension(RuntimeEditorExtension *extension);

private:
	static void render_ui();
	static void render_window_save_world();
	static void render_window_visualizer();
	static void render_window_hotkeys();
	static void update_selection();

	static void clear_ui_timers();
	static void update_ui_timers();
};
