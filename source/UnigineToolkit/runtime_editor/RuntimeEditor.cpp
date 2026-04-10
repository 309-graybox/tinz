// Copyright (C), UNIGINE. All rights reserved.

#include "RuntimeEditor.h"

#include "../imgui/ImGuiImpl.h"
#include "../ui/elements/Canvas.h"
#include "CameraSettingsWindow.h"
#include "ImGuiManipulator.h"
#include "NodesWindow.h"
#include "PropertiesWindow.h"
#include "USCWindow.h"
#include "WidgetsWindow.h"

#include <UnigineComponentSystem.h>
#include <UnigineConsole.h>
#include <UnigineEditor.h>
#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineMap.h>
#include <UnigineNode.h>
#include <UnigineProfiler.h>
#include <UnigineThread.h>
#include <UnigineVector.h>
#include <UnigineVisualizer.h>

using namespace Unigine;
using namespace Math;

namespace {
// common
Input::KEY enable_hotkey = Input::KEY::KEY_F2;
bool enabled = false;
bool will_enable = false;
PlayerPtr saved_player;
EditorPlayer *editor_player = nullptr;
Input::MOUSE_HANDLE saved_mouse_handle;
int saved_gui_mouse_grab;
bool saved_gui_mouse_enabled;
bool saved_input_mouse_grab;
bool saved_input_cursor_hide;

// viewport
Vector<NodePtr> selected_nodes;
float wireframe_selection_timer = 0;
Map<String /*group*/, bool /*enabled*/> visualizer_groups;
Unigine::Mutex visualizer_groups_mutex;

// ui
bool hide_game_ui = true;
Vector<Pair<WidgetPtr /*ui*/, bool /*enabled*/>> engine_widgets_ui;
Vector<Pair<UI::CanvasPtr /*ui*/, bool /*enabled*/>> toolkit_ui;

// windows
bool windows_save_world_open = false;
bool windows_nodes_open = false;
bool windows_properties_open = false;
bool windows_visualizer_open = false;
bool windows_widgets_open = false;
bool windows_usc_open = false;
bool windows_camera_settings_open = false;
bool windows_hotkeys_open = false;

// manipulators
const char *manipulator_items[] = {"Select", "Move", "Rotate", "Scale"};
int manipulator_mode = 0;
const char *pivot_items[] = {"Center", "Pivot"};
int pivot_mode = 1;
const char *orientation_items[] = {"World", "Parent", "Local"};
int orientation_mode = 0;
bool manipulator_hovered = false;

// game start/stop
float time_speed = 1.0f;
bool game_enabled = true;
bool physics_enabled = true;

// extensions
Vector<RuntimeEditorExtension *> extensions;
}	 // namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RuntimeEditor::init()
{
	enabled = false;

	NodesWindow::init();
	PropertiesWindow::init();
	WidgetsWindow::init();
	USCWindow::init();
	CameraSettingsWindow::init();

	saved_mouse_handle = Input::getMouseHandle();
	selected_nodes.clear();
	visualizer_groups.clear();
}

void RuntimeEditor::update()
{
	// check if selected nodes valid
	for (int i = 0; i < selected_nodes.size(); i++)
	{
		if (selected_nodes[i].isNull())
			selected_nodes.remove(i--);
	}

	if (!Console::isActive() && Input::isKeyDown(enable_hotkey))
		will_enable = !enabled;

	if (will_enable != enabled)
	{
		enabled = will_enable;

		if (enabled)
		{
			Visualizer::setEnabled(true);

			// show mouse cursor
			saved_mouse_handle = Input::getMouseHandle();
			saved_gui_mouse_grab = Gui::getCurrent()->getMouseGrab();
			saved_gui_mouse_enabled = Gui::getCurrent()->isMouseEnabled();
			saved_input_mouse_grab = Input::isMouseGrab();
			saved_input_cursor_hide = Input::isMouseCursorHide();
			Input::setMouseHandle(Input::MOUSE_HANDLE_USER);
			Gui::getCurrent()->setMouseGrab(false);
			Gui::getCurrent()->setMouseEnabled(true);
			Input::setMouseGrab(false);
			Input::setMouseCursorHide(false);

			// change current player
			saved_player = Game::getPlayer();
			if (!editor_player)
				editor_player = new EditorPlayer();
			editor_player->setWorldTransform(saved_player->getWorldTransform());
			Game::setPlayer(editor_player->getPlayer());

			// disable component system
			// ComponentSystem::get()->setEnabled(false);

			// disable player's controls
			// ...

			// store game ui state
			engine_widgets_ui.clear();
			WidgetVBoxPtr vbox = Gui::getCurrent()->getVBox();
			for (int i = 0; i < vbox->getNumChildren(); i++)
			{
				WidgetPtr w = vbox->getChild(i);
				if (w->getType() == Widget::WIDGET_ENGINE)
					continue;
				engine_widgets_ui.append(MakePair(w, w->isHidden()));
				if (hide_game_ui)
					w->setHidden(true);
			}

			toolkit_ui.clear();
			auto &canvases = UI::Canvas::get()->getAllCanvases();
			for (int i = 0; i < canvases.size(); i++)
			{
				auto &canvas = canvases[i];
				if (canvas->render_mode.get() == 0 /*screen*/)
				{
					toolkit_ui.append(MakePair(canvas->getPtr(), canvas->isEnabled()));
					if (hide_game_ui)
						canvas->setEnabled(false);
				}
			}

			// show debug window
			ImGuiImpl::getWidget()->setHidden(false);
			ImGuiImpl::bringToFront();
		}
		else
		{
			Visualizer::setEnabled(false);

			// restore player and input
			Input::setMouseHandle(saved_mouse_handle);
			Gui::getCurrent()->setMouseGrab(saved_gui_mouse_grab);
			Gui::getCurrent()->setMouseEnabled(saved_gui_mouse_enabled);
			Input::setMouseGrab(saved_input_mouse_grab);
			Input::setMouseCursorHide(saved_input_cursor_hide);
			if (saved_player)
				Game::setPlayer(saved_player);

			// enable player's controls
			// ...

			// enable component system
			// gd::ComponentSystem::get()->setEnabled(true);

			// restore game ui state
			for (int i = 0; i < engine_widgets_ui.size(); i++)
				if (engine_widgets_ui[i].first)
					engine_widgets_ui[i].first->setHidden(engine_widgets_ui[i].second);
			for (int i = 0; i < toolkit_ui.size(); i++)
				if (toolkit_ui[i].first)
					toolkit_ui[i].first->setEnabled(toolkit_ui[i].second);

			// hide manipulator
			ImGuiManipulator::begin();
			ImGuiManipulator::end();

			// hide debug window
			ImGuiImpl::getWidget()->setHidden(true);
		}
	}

	if (enabled)
	{
		// logic
		update_selection();
		editor_player->update();

		// manipulators
		ImGuiManipulator::begin();
		if (manipulator_mode != 0 /*Selection*/ && selected_nodes.size())
		{
			int widget_type = manipulator_mode == 1
								  ? Widget::WIDGET_MANIPULATOR_TRANSLATOR
								  : (manipulator_mode == 2 ? Widget::WIDGET_MANIPULATOR_ROTATOR
														   : Widget::WIDGET_MANIPULATOR_SCALER);

			NodePtr &node = selected_nodes.last();

			// set coordinate system
			Mat4 basis = Mat4_identity; /*World*/
			if (orientation_mode == 1 /*Parent*/)
			{
				NodePtr parent = node->getParent();
				if (parent)
					basis = parent->getWorldTransform();
			}
			else if (orientation_mode == 2 /*Local*/)
			{
				basis = node->getWorldTransform();
				// if (pivot_mode == 0 /*Center*/)
				//	basis.setColumn3(3, get_nodes_bound_center(selected_nodes));
			}

			// show manipulator
			Mat4 transform = node->getWorldTransform();
			if (ImGuiManipulator::show(transform, widget_type, basis))
				node->setWorldTransform(transform);
		}
		manipulator_hovered = ImGuiManipulator::isHovered();
		ImGuiManipulator::end();

		if (hide_game_ui)
		{
			// protect from appearing game ui during debugging
			WidgetVBoxPtr vbox = Gui::getCurrent()->getVBox();
			for (int i = 0; i < vbox->getNumChildren(); i++)
			{
				WidgetPtr w = vbox->getChild(i);
				if (w != ImGuiImpl::getWidget()
					&& w->getType() != Widget::WIDGET_MANIPULATOR_TRANSLATOR
					&& w->getType() != Widget::WIDGET_MANIPULATOR_ROTATOR
					&& w->getType() != Widget::WIDGET_MANIPULATOR_SCALER
					&& w->getType() != Widget::WIDGET_ENGINE)
					w->setHidden(true);
			}
		}

		// move ImGui in front of background manipulators
		if (manipulator_mode != 0 /*Selection*/)
			ImGuiImpl::bringToFront();

		// ui
		ImGuiImpl::newFrame();
		render_ui();
		ImGuiImpl::render();
	}
}

void RuntimeEditor::render_ui()
{
	update_ui_timers();

	// Warning! Don't put the following code at the end of this function.
	// USCWindow uses ImGuiColorTextEdit that must be updated before
	// any other windows. Otherwise isKeyboardAvailable() will not work!
	if (windows_usc_open)
		USCWindow::render(&windows_usc_open);

	///////////////////////////////////////////////////////////////////////////////////
	// Hotkeys
	///////////////////////////////////////////////////////////////////////////////////

	if (isKeyboardAvailable())
	{
		// windows
		if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_S))
			windows_save_world_open = true;
		if (Input::isKeyDown(Input::KEY_N))
			windows_nodes_open = !windows_nodes_open;
		if (Input::isKeyDown(Input::KEY_P))
			windows_properties_open = !windows_properties_open;
		if (Input::isKeyDown(Input::KEY_V))
			windows_visualizer_open = !windows_visualizer_open;
		if (Input::isKeyDown(Input::KEY_G))
			windows_widgets_open = !windows_widgets_open;
		if (Input::isKeyDown(Input::KEY_U))
			windows_usc_open = !windows_usc_open;
		if (Input::isKeyDown(Input::KEY_M))
			windows_camera_settings_open = !windows_camera_settings_open;

		// manipulator
		if (!Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT) /*If free camera not enabled*/)
		{
			if (Input::isKeyDown(Input::KEY_Q))
				manipulator_mode = 0;
			if (Input::isKeyDown(Input::KEY_W))
				manipulator_mode = 1;
			if (Input::isKeyDown(Input::KEY_E))
				manipulator_mode = 2;
			if (Input::isKeyDown(Input::KEY_R))
				manipulator_mode = 3;

			if (Input::isKeyDown(Input::KEY_Z))
			{
				pivot_mode = (pivot_mode + 1) % 2;
			}
			if (Input::isKeyDown(Input::KEY_X))
			{
				orientation_mode = (orientation_mode + 1) % 3;
			}
		}

		// game start/stop
		if (Input::isKeyDown(Input::KEY_SPACE))
		{
			if (game_enabled)
			{
				time_speed = 0.0f;
				game_enabled = false;
				physics_enabled = false;
			}
			else
			{
				time_speed = 1.0f;
				game_enabled = true;
				physics_enabled = true;
			}
		}

		// extensions
		for (auto &e : extensions)
			e->processHotkeys();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Main Menu Bar
	///////////////////////////////////////////////////////////////////////////////////
	auto window = WindowManager::getMainWindow();
	if (!window)
		return;
	Unigine::Math::ivec2 window_size = window->getClientSize();
	window_size = Unigine::Math::ivec2(
		Unigine::Math::vec2(window_size) * WindowManager::getMainWindow()->getDpiScale());
	if (window_size.length2() <= 1)
		return;

	ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration;
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(itof(window_size.x), 0.0f));
	ImGui::Begin("Debugger", nullptr, flags);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Reload World"))
				World::reloadWorld();
			if (ImGui::MenuItem("Save World As...", "Ctrl + S"))
				windows_save_world_open = true;
			ImGui::Separator();
			if (ImGui::MenuItem("Quit World"))
				World::quitWorld();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows"))
		{
			ImGui::MenuItem("Nodes", "N", &windows_nodes_open);
			ImGui::MenuItem("Properties", "P", &windows_properties_open);
			ImGui::MenuItem("Visualizer", "V", &windows_visualizer_open);
			ImGui::MenuItem("GuiWidgets", "G", &windows_widgets_open);
			ImGui::MenuItem("UnigineScript", "U", &windows_usc_open);
			ImGui::MenuItem("Camera Settings", "M", &windows_camera_settings_open);
			for (auto &e : extensions)
				e->renderMenuWindows();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Hotkeys", nullptr, &windows_hotkeys_open);
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	///////////////////////////////////////////////////////////////////////////////////
	// Manipulator
	///////////////////////////////////////////////////////////////////////////////////

	ImGui::SetNextItemWidth(70.0f);
	ImGui::Combo(
		"##Manipulator", &manipulator_mode, manipulator_items, IM_ARRAYSIZE(manipulator_items));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70.0f);
	ImGui::Combo("##Point", &pivot_mode, pivot_items, IM_ARRAYSIZE(pivot_items));
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70.0f);
	ImGui::Combo(
		"##CoordSystem", &orientation_mode, orientation_items, IM_ARRAYSIZE(orientation_items));
	ImGui::SameLine();

	///////////////////////////////////////////////////////////////////////////////////
	// Camera
	///////////////////////////////////////////////////////////////////////////////////

	ImGui::TextUnformatted("|");
	ImGui::SameLine();

	static float cam_speed;
	cam_speed = editor_player->getVelocity();
	ImGui::SetNextItemWidth(50.0f);
	if (ImGui::InputFloat("Camera Speed", &cam_speed, 0, 0, "%.3f"))
		editor_player->setVelocity(cam_speed);
	ImGui::SameLine();

	static float stored_speed[3]{5.0f, 50.0f, 500.0f};
	static int speed_mode = 0;
	stored_speed[speed_mode] = cam_speed;
	bool speed_changed = false;
	speed_changed = ImGui::RadioButton("1", &speed_mode, 0);
	ImGui::SameLine();
	speed_changed |= ImGui::RadioButton("2", &speed_mode, 1);
	ImGui::SameLine();
	speed_changed |= ImGui::RadioButton("3", &speed_mode, 2);
	ImGui::SameLine();
	if (isKeyboardAvailable())
	{
		if (Input::isKeyDown(Input::KEY_DIGIT_1))
		{
			speed_mode = 0;
			speed_changed = true;
		}
		if (Input::isKeyDown(Input::KEY_DIGIT_2))
		{
			speed_mode = 1;
			speed_changed = true;
		}
		if (Input::isKeyDown(Input::KEY_DIGIT_3))
		{
			speed_mode = 2;
			speed_changed = true;
		}
	}
	if (speed_changed)
		editor_player->setVelocity(stored_speed[speed_mode]);

	static vec3 camera_pos;
	camera_pos = vec3(editor_player->getPlayer()->getWorldPosition());
	ImGui::SetNextItemWidth(170.0f);
	if (ImGui::InputFloat3("##Camera Position", camera_pos.get(), "%.2f"))
		editor_player->setWorldPosition(Vec3(camera_pos));
	ImGui::SameLine();

	///////////////////////////////////////////////////////////////////////////////////
	// Time
	///////////////////////////////////////////////////////////////////////////////////

	ImGui::TextUnformatted("|");
	ImGui::SameLine();

	if (ImGui::Button("Play", ImVec2(60.0f, 0.0f)))
	{
		time_speed = 1.0f;
		game_enabled = true;
		physics_enabled = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop", ImVec2(60.0f, 0.0f)))
	{
		time_speed = 0.0f;
		game_enabled = false;
		physics_enabled = false;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70.0f);
	ImGui::Checkbox("Game", &game_enabled);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Game Enabled");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(70.0f);
	ImGui::Checkbox("Physics", &physics_enabled);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Physics Enabled");
	ImGui::SameLine();
	if (ImGui::Button("T"))
	{
		time_speed = 10.0f;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Timelapse");
	ImGui::SameLine();
	if (ImGui::Button("N"))
	{
		time_speed = 1.0f;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Normal Speed");
	ImGui::SameLine();
	if (ImGui::Button("S"))
	{
		time_speed = 0.1f;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Slow Speed");
	ImGui::SameLine();
	if (ImGui::Button("E"))
	{
		time_speed = 0.0001f;
	}
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Extra Slow Speed");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(60.0f);
	ImGui::InputFloat("Time Speed", &time_speed, 0, 0, "%.5f");
	ImGui::SameLine();

	Game::setScale(time_speed);
	Game::setEnabled(game_enabled);
	Physics::setScale(time_speed);
	Physics::setEnabled(physics_enabled);

	///////////////////////////////////////////////////////////////////////////////////
	// Game UI
	///////////////////////////////////////////////////////////////////////////////////

	ImGui::TextUnformatted("|");
	ImGui::SameLine();

	if (ImGui::Checkbox("Hide UI", &hide_game_ui))
	{
		for (int i = 0; i < engine_widgets_ui.size(); i++)
			engine_widgets_ui[i].first->setHidden(
				hide_game_ui ? true : engine_widgets_ui[i].second);
		for (int i = 0; i < toolkit_ui.size(); i++)
			if (toolkit_ui[i].first)
				toolkit_ui[i].first->setEnabled(hide_game_ui ? false : toolkit_ui[i].second);
		ImGuiImpl::getWidget()->setHidden(false);
	}

	ImGui::End();

	///////////////////////////////////////////////////////////////////////////////////
	// Windows
	///////////////////////////////////////////////////////////////////////////////////

	render_window_save_world();
	render_window_visualizer();
	render_window_hotkeys();
	if (windows_nodes_open)
		NodesWindow::render(&windows_nodes_open);
	if (windows_properties_open)
		PropertiesWindow::render(&windows_properties_open);
	if (windows_widgets_open)
		WidgetsWindow::render(&windows_widgets_open);
	if (windows_camera_settings_open)
		CameraSettingsWindow::render(&windows_camera_settings_open);
	for (auto &e : extensions)
		e->renderWindow();
	//  Warning! Unfortunately, we can't put the following code here. This code is written
	//  at the beginning of this function.
	//	if (windows_usc_open)
	//		USCWindow::render(&windows_usc_open);
}

void RuntimeEditor::render_window_save_world()
{
	if (!windows_save_world_open)
		return;

	ImGui::Begin("Save World As...", &windows_save_world_open);

	static char str[128] = "debug.world";
	ImGui::InputText("File Name", str, IM_ARRAYSIZE(str));
	static bool save_hidden = true;
	ImGui::Checkbox("Save all nodes (runtime, hidden, cache, etc.)", &save_hidden);

	if (ImGui::Button("Save World"))
	{
		if (save_hidden)
		{
			Vector<NodePtr> nodes;
			World::getNodes(nodes);
			for (int i = 0; i < nodes.size(); i++)
			{
				NodePtr &n = nodes[i];
				n->setSaveToWorldEnabled(true);
				n->setShowInEditorEnabled(true);
			}
		}

		World::saveWorld(str);
	}
	ImGui::Text(
		"Warning! This button will change all NodeLayers in this world! Don't commit anything "
		"after save. For debug reasons only.");

	ImGui::End();
}

void RuntimeEditor::render_window_visualizer()
{
	if (!windows_visualizer_open)
		return;

	ImGui::Begin("Visualizer", &windows_visualizer_open);
	if (ImGui::Button("Check All"))
		for (auto it = visualizer_groups.begin(); it != visualizer_groups.end(); ++it)
			it->data = true;
	ImGui::SameLine();
	if (ImGui::Button("Uncheck All"))
		for (auto it = visualizer_groups.begin(); it != visualizer_groups.end(); ++it)
			it->data = false;
	if (ImGui::Button("Clear"))
		Visualizer::clear();

	// show checkboxes
	for (auto it = visualizer_groups.begin(); it != visualizer_groups.end(); ++it)
		ImGui::Checkbox(it->key, &it->data);
	ImGui::End();
}

void RuntimeEditor::render_window_hotkeys()
{
	if (!windows_hotkeys_open)
		return;

	ImGui::Begin("Hotkeys", &windows_hotkeys_open);
	ImGui::TextUnformatted(
		"Debugger:\n"
		"F2 - Show/Hide debugger\n"
		"Space - Play/Stop the game\n"
		"\n"
		"Viewport:\n"
		"LMB - Select node\n"
		"Shift + LMB - Add to selection\n"
		"Ctrl + LMB - Remove from selection\n"
		"F - Focus to selection\n"
		"Shift + F - Focus and attach to selection\n"
		"Q,W,E,R - Manipulator modes\n"
		"Z - Pivot point modes\n"
		"X - Coordinate system modes\n"
		"\n"
		"Camera:\n"
		"RMB - Free (WSAD,QE,Shift)\n"
		"Alt + LMB - Orbit\n"
		"Alt + RMB - Dolly\n"
		"MMB - Pan\n"
		"1,2,3 - Change Speed Preset\n");
	ImGui::End();
}

void RuntimeEditor::shutdown()
{
	ImGuiManipulator::destroy();

	NodesWindow::shutdown();
	PropertiesWindow::shutdown();
	WidgetsWindow::shutdown();
	USCWindow::shutdown();
	CameraSettingsWindow::shutdown();

	clear_ui_timers();

	Input::setMouseHandle(saved_mouse_handle);
	if (editor_player)
	{
		delete editor_player;
		editor_player = nullptr;
	}
}

void RuntimeEditor::setHotkeyEnabled(Input::KEY hotkey)
{
	enable_hotkey = hotkey;
}

Input::KEY RuntimeEditor::getHotkeyEnabled()
{
	return enable_hotkey;
}

void RuntimeEditor::setEnabled(bool enable)
{
	will_enable = enable;
}

bool RuntimeEditor::isEnabled()
{
	return enabled;
}

bool RuntimeEditor::isMouseAvailable()
{
	return !Console::isActive() && !ImGuiImpl::isWantCaptureMouse() && !manipulator_hovered;
}

bool RuntimeEditor::isKeyboardAvailable()
{
	return !Console::isActive() && !ImGuiImpl::isWantCaptureKeyboard();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Selections
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RuntimeEditor::clearNodeSelection()
{
	selected_nodes.clear();
}

void RuntimeEditor::selectNode(const NodePtr &node)
{
	selected_nodes.clear();
	selected_nodes.append(node);
}

void RuntimeEditor::selectNodes(const Vector<NodePtr> &nodes)
{
	selected_nodes = nodes;
}

bool RuntimeEditor::isSelectedNode()
{
	return selected_nodes.size() > 0;
}

NodePtr RuntimeEditor::getSelectedNode()
{
	if (selected_nodes.size())
		return selected_nodes.last();
	return NodePtr();
}

void RuntimeEditor::getSelectedNodes(Vector<NodePtr> &nodes)
{
	nodes = selected_nodes;
}

void RuntimeEditor::update_selection()
{
	// selection
	if (Input::isMouseButtonDown(Input::MOUSE_BUTTON_LEFT) && isMouseAvailable()
		&& !Input::isKeyPressed(Input::KEY_ANY_ALT))
	{
		ivec2 mouse_pos = Input::getMousePosition();

		auto window = WindowManager::getMainWindow();
		if (!window)
			return;

		Unigine::Math::ivec2 window_size = window->getClientSize();
		window_size = Unigine::Math::ivec2(
			Unigine::Math::vec2(window_size) * WindowManager::getMainWindow()->getDpiScale());
		Unigine::Math::ivec2 window_pos = window->getClientPosition();

		ivec2 win_pouse_pos = mouse_pos - window_pos;
		if (win_pouse_pos.x >= 0 && win_pouse_pos.y >= 0 && win_pouse_pos.x < window_size.x
			&& win_pouse_pos.y < window_size.y)
		{
			const PlayerDummyPtr &p = editor_player->getPlayer();
			Vec3 pos = p->getWorldPosition();
			Vec3 dir = Vec3(p->getDirectionFromMainWindow(mouse_pos.x, mouse_pos.y));
			NodePtr obj =
				Editor::getIntersection(pos + dir * p->getZNear(), pos + dir * p->getZFar());
			if (obj)
			{
				// add to selection
				if (Input::isKeyPressed(Input::KEY_ANY_SHIFT))
				{
					selected_nodes.append(obj);
					wireframe_selection_timer = 1.0f;
				}
				// remove from selection
				else if (Input::isKeyPressed(Input::KEY_ANY_CTRL))
				{
					int index = selected_nodes.findIndex(obj);
					if (index != -1)
					{
						selected_nodes.remove(index);
					}
				}
				// select
				else
				{
					selected_nodes.clear();
					selected_nodes.append(obj);
					wireframe_selection_timer = 1.0f;
				}
			}
			else if (!Input::isKeyPressed(Input::KEY_ANY_SHIFT)
					 && !Input::isKeyPressed(Input::KEY_ANY_CTRL))
				clearNodeSelection();
		}
	}

	// focus
	if (Input::isKeyDown(Input::KEY_F) && isKeyboardAvailable())
	{
		if (Input::isKeyPressed(Input::KEY_ANY_SHIFT) && selected_nodes.size())
			editor_player->focus(selected_nodes.last(), true);	  // focus and attach
		else
			editor_player->focus(selected_nodes);	 // just focus
	}

	// show wireframe of selection
	if (wireframe_selection_timer > 0)
	{
		wireframe_selection_timer -= Engine::get()->getIFps();
		if (wireframe_selection_timer < 0)
			wireframe_selection_timer = 0;

		for (int i = 0; i < selected_nodes.size(); i++)
		{
			NodePtr &node = selected_nodes[i];
			if (node->getType() >= Node::OBJECT_BEGIN && node->getType() <= Node::OBJECT_END)
			{
				Visualizer::renderObject(
					static_ptr_cast<Object>(node), vec4(0, 1, 0, wireframe_selection_timer));
			}
		}
	}

	// show bound boxes
	for (int i = 0; i < selected_nodes.size(); i++)
		Visualizer::renderNodeBoundBox(selected_nodes[i], vec4_green);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Visualizer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool RuntimeEditor::isAllowToUseVisualizer(const char *group)
{
	if (!Visualizer::isEnabled())
		return false;	 // A little bit optimization. If we don't see anything, why we should call
						 // Visualizer?
	if (!isEnabled())
		return true;	// If we don't use Debugger then we can use Visualizer as usual, without any
						// restrictions

	ScopedLock lock(visualizer_groups_mutex);

	auto it = visualizer_groups.find(group);
	if (it == visualizer_groups.end())
	{
		visualizer_groups.append(group, true);
		return true;
	}
	else
	{
		return it->data;
	}
}

void RuntimeEditor::setAllowToUseVisualizer(const char *group, bool value)
{
	ScopedLock lock(visualizer_groups_mutex);

	auto it = visualizer_groups.find(group);
	if (it == visualizer_groups.end())
		visualizer_groups.append(group, value);
	else
		visualizer_groups.insert(group, value);
}

void RuntimeEditor::renderBox(const char *group, const vec3 &size, const Mat4 &transform,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderBox(size, transform, color, duration, depth_test);
}

void RuntimeEditor::renderPoint3D(const char *group, const Vec3 &v, float size, const vec4 &color,
	bool screen_space, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderPoint3D(v, size, color, screen_space, duration, depth_test);
}

void RuntimeEditor::renderTriangle3D(const char *group, const Vec3 &v0, const Vec3 &v1,
	const Vec3 &v2, const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderTriangle3D(v0, v1, v2, color, duration, depth_test);
}

void RuntimeEditor::renderLine3D(const char *group, const Vec3 &v0, const Vec3 &v1,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderLine3D(v0, v1, color, duration, depth_test);
}

void RuntimeEditor::renderLine2D(const char *group, const vec2 &v0, const vec2 &v1,
	const vec4 &color, float order, float duration)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderLine2D(v0, v1, color, order, duration);
}

void RuntimeEditor::renderVector(const char *group, const Vec3 &position_start,
	const Vec3 &position_end, const vec4 &color, float arrow_size, bool screen_space,
	float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderVector(
			position_start, position_end, color, arrow_size, screen_space, duration, depth_test);
}

void RuntimeEditor::renderCircle(const char *group, float radius, const Mat4 &transform,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderCircle(radius, transform, color, duration, depth_test);
}

void RuntimeEditor::renderSphere(const char *group, float radius, const Mat4 &transform,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderSphere(radius, transform, color, duration, depth_test);
}

void RuntimeEditor::renderCylinder(const char *group, float radius, float height,
	const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
	bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderCylinder(radius, height, transform, color, duration, depth_test);
}

void RuntimeEditor::renderCapsule(const char *group, float radius, float height,
	const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
	bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderCapsule(radius, height, transform, color, duration, depth_test);
}

void RuntimeEditor::renderSolidBox(const char *group, const vec3 &size, const Mat4 &transform,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderSolidBox(size, transform, color, duration, depth_test);
}

void RuntimeEditor::renderSolidSphere(const char *group, float radius, const Mat4 &transform,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderSolidSphere(radius, transform, color, duration, depth_test);
}

void RuntimeEditor::renderSolidCylinder(const char *group, float radius, float height,
	const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
	bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderSolidCylinder(radius, height, transform, color, duration, depth_test);
}

void RuntimeEditor::renderSolidCapsule(const char *group, float radius, float height,
	const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
	bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderSolidCapsule(radius, height, transform, color, duration, depth_test);
}

void RuntimeEditor::renderMessage2D(const char *group, const vec3 &position, const vec3 &center,
	const char *str, const vec4 &color, int outline, int font_size, float duration)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderMessage2D(position, center, str, color, outline, font_size, duration);
}

void RuntimeEditor::renderMessage3D(const char *group, const Vec3 &position, const vec3 &center,
	const char *str, const vec4 &color, int outline, int font_size, float duration)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderMessage3D(position, center, str, color, outline, font_size, duration);
}

void RuntimeEditor::renderBoundBox(const char *group, const BoundBox &box, const Mat4 &transform,
	const vec4 &color, float duration, bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderBoundBox(box, transform, color, duration, depth_test);
}

void RuntimeEditor::renderBoundSphere(const char *group, const Unigine::Math::BoundSphere &sphere,
	const Unigine::Math::Mat4 &transform, const Unigine::Math::vec4 &color, float duration,
	bool depth_test)
{
	if (isAllowToUseVisualizer(group))
		Visualizer::renderBoundSphere(sphere, transform, color, duration, depth_test);
}

void RuntimeEditor::focusToNode(const Unigine::NodePtr &node, bool attach)
{
	editor_player->focus(node, attach);
}

void RuntimeEditor::setTimeSpeed(float speed)
{
	time_speed = speed;
	if (compare(speed, 0.f))
		game_enabled = physics_enabled = false;
}

void RuntimeEditor::setCameraTransform(const Mat4 &transform)
{
	editor_player->setWorldTransform(transform);
}

namespace {
double now;
Unigine::Vector<double> timers;
}	 // namespace

int RuntimeEditor::addUiTimer()
{
	int timer_id = timers.size();
	timers.resize(timer_id + 1);
	return timer_id;
}

void RuntimeEditor::clearUiTimeout(int timer_id)
{
	timers[timer_id] = -1;
}

void RuntimeEditor::setUiTimeout(int timer_id, double duration)
{
	timers[timer_id] = now + duration;
}

bool RuntimeEditor::checkUiTimeout(int timer_id)
{
	const double &t = timers[timer_id];
	return t >= 0 && t <= now;
}

void RuntimeEditor::clear_ui_timers()
{
	timers.clear();
}

void RuntimeEditor::update_ui_timers()
{
	now = ImGui::GetTime();
}

void RuntimeEditor::addExtension(RuntimeEditorExtension *extension)
{
	extensions.appendUnique(extension);
}

void RuntimeEditor::removeExtension(RuntimeEditorExtension *extension)
{
	extensions.removeOne(extension);
}
