#pragma once
#include "../elements/Canvas.h"
#include "../elements/Element.h"
#include "UndoManager.h"

#include <UnigineInput.h>
#include <UnigineJson.h>
#include <UnigineLogic.h>
#include <UnigineStreams.h>
#include <UnigineVector.h>
#include <UnigineWidgets.h>

class ChangedWidgetCommand;

class UIDesigner
{
public:
	UIDesigner() = default;

	static UIDesigner *get();

	static void setHotkeyEnabled(Unigine::Input::KEY hotkey);
	static Unigine::Input::KEY getHotkeyEnabled();

	void init(const Unigine::GuiPtr &gui, bool use_custom_render = false);
	void init(bool use_custom_render = false);
	void update();
	void shutdown();

	void setShow(bool in_show);
	bool isShow() const { return show; }

	bool isInitialized() const { return initialized; }

private:
	friend class DesignerEditorLogic;

	bool initialized = false;

#ifndef EDITOR_PLUGIN
	void create_trash_root();
#endif

	void update_show_state();
	void clear_canvas();

	void render_main_menu();
	void render_toolset();
	void render_hierarchy();
	void render_parameters();
	void render_hotkeys();

	// parameters
	void render_base_parameters(const Unigine::Vector<UI::ElementPtr> &elements,
		bool &parameter_changed, bool &parameter_released);
	void render_derived_parameters(const Unigine::Vector<UI::ElementPtr> &elements,
		bool &parameter_changed, bool &parameter_released);
	void render_sprite_shader_parameters(const Unigine::Vector<UI::ElementPtr> &elements,
		bool &parameter_changed, bool &parameter_released);

	// hierarchy
	void render_hierarchy_element(const UI::ElementPtr &element);
	void select_element_by_user(const UI::ElementPtr &element, bool update_hierarchy_window = true);
	void select_element(const UI::ElementPtr &element, bool update_hierarchy_window = true);
	void select_elements(
		const Unigine::Vector<UI::ElementPtr> &elements, bool update_hierarchy_window = true);
	void sort_and_select_elements(const Unigine::Vector<UI::ElementPtr> &elements);
	void refresh_selected_elements_root();
	void refresh_selected_elements_parents();

#ifdef EDITOR_PLUGIN
	bool window_toolset = false;
	bool window_hierarchy = false;
	bool window_parameters = false;
	bool window_hotkeys = false;
#else
	bool window_toolset = true;
	bool window_hierarchy = true;
	bool window_parameters = true;
	bool window_hotkeys = true;
#endif
	bool need_to_reset_layout = false;

	bool need_to_focus = false;	   // update hierarchy window

	bool show = false;
	Unigine::Input::MOUSE_HANDLE saved_mouse_handle = Unigine::Input::MOUSE_HANDLE_GRAB;

	Unigine::Math::ivec2 window_size;
	float dpi_scale = 1.0f;

	// window - Editor's window coordinates
	// screen - canvas' render coordinates (screen)
	// canvas - canvas's coordinates (reference)
	Unigine::Math::vec2 norm_to_window(const Unigine::Math::vec2 &norm_pos) const;
	Unigine::Math::vec2 window_to_screen(const Unigine::Math::vec2 &pos) const;
	Unigine::Math::vec4 window_to_screen(const Unigine::Math::vec4 &rect) const;
	Unigine::Math::vec2 canvas_to_screen(const Unigine::Math::vec2 &pos) const;
	Unigine::Math::vec2 screen_to_canvas(const Unigine::Math::vec2 &pos) const;
	Unigine::Math::vec2 window_to_canvas(const Unigine::Math::vec2 &pos) const;
	Unigine::Math::vec2 canvas_to_window(const Unigine::Math::vec2 &pos) const;
	Unigine::Math::vec2 canvas_to_norm(const Unigine::Math::vec2 &pos) const;

	float convert_window_to_canvas(float window_pos) const;
	float convert_canvas_to_window(float canvas_pos) const;

	// canvas (current selected, aka project)
	UI::CanvasPtr canvas;
	void create_canvas_node();

	// creation mode
	Unigine::Vector<Unigine::String> all_creator_elements;
	Unigine::String selected_creator;
	void find_all_creator_elements();

	// selection mode
	struct Manipulator;
	void check_node_selection();
	void mouse_update();
	void mouse_context_menu_update();
	void keyboard_update();
	bool is_selection_mode();
	void selection_mode_update();
	void creation_mode_update();
	void check_selection_existence();
	void update_selection_parameters();
	void update_manipulator();
	bool update_manipulator_select(
		const Unigine::Math::vec2 &mouse_pos, const Manipulator &manipulator);
	bool update_manipulator_select(const Unigine::Math::vec2 &mouse_pos,
		const Unigine::Math::vec2 &p0, const Unigine::Math::vec2 &p1, const Unigine::Math::vec2 &p2,
		const Unigine::Math::vec2 &p3);
	void cut_selected();
	void copy_to_clipboard();
	void paste_from_clipboard();
	void duplicate_selected();
	void destroy_selected();
	void select_all();
	void deselect_all();
	void move_element_by_keyboard();
	void update_canvas();
	void draw_manipulator();
	void draw_circle(
		int id, const Unigine::Math::vec3 &point, float radius, const Unigine::Math::vec4 &color);
	void draw_rect_selection();
	void draw_selected_element_pivot();
	void draw_selected_element_anchor();
	void draw_align_line_horizontal(float y);
	void draw_align_line_vertical(float x);
	void clear_selection();
	struct ElementDepth
	{
		UI::ElementPtr element;
		int depth;
	};
	void find_elements_depth(const UI::ElementPtr &parent, float px_x, float px_y,
		Unigine::Vector<ElementDepth> &out_elements, int depth = 0) const;
	UI::ElementPtr find_element(float px_x, float px_y) const;
	UI::ElementPtr find_element(const UI::ElementPtr &parent, float px_x, float px_y) const;
	UI::ElementPtr find_next_element(
		float px_x, float px_y, const UI::ElementPtr &prev_element) const;
	UI::ElementPtr find_next_element(const UI::ElementPtr &parent, float px_x, float px_y,
		const UI::ElementPtr &prev_element) const;
	Unigine::Vector<UI::ElementPtr> find_elements(const Unigine::Math::vec4 &rect) const;
	void find_elements(const UI::ElementPtr &parent, const Unigine::Math::vec4 &rect,
		Unigine::Vector<UI::ElementPtr> &out_result) const;

	Unigine::Vector<UI::ElementPtr> selected_elements;		   // all selected elements
	Unigine::Vector<UI::ElementPtr> prev_selected_elements;	   // previous selection
	Unigine::Vector<UI::ElementPtr> selected_roots;		 // common roots of all selected elements
	Unigine::Vector<UI::ElementPtr> selected_parents;	 // all parents of all selected elements

	bool skip_parameter_window = false;

	Unigine::Vector<Unigine::NodePtr> clipboard;	// copy-paste

	struct DragSelectState
	{
		UI::ElementPtr pending_element;
		Unigine::Math::vec2 mouse_down_pos;
		bool waiting_for_drag = false;
	};
	DragSelectState drag_select;
	const float drag_threshold = 3.0f;	  // pixels

	UndoManager *undo_manager = nullptr;
	friend class CreatedWidgetCommand;
	friend class SelectWidgetCommand;
	friend class ChangedWidgetCommand;
	friend class DeleteWidgetCommand;
#ifndef EDITOR_PLUGIN
	UI::ElementPtr trash_root;
#endif
	ChangedWidgetCommand *selected_element_changes = nullptr;
	bool widget_moved = false;
	bool parameter_changed = false;
	bool parameter_released = false;
	void refresh_selected_element_changes();

	int render_width = 1920;
	int render_height = 1080;
	Unigine::Math::vec3 gui_sprite_pos_scale = Unigine::Math::vec3(0, 0.035f, 0.75f);
	Unigine::WidgetSpritePtr background;
	Unigine::WidgetSpritePtr gui_sprite;
	Unigine::WidgetCanvasPtr canvas_widget;
	Unigine::Math::vec4 gui_sprite_bg_color = Unigine::Math::vec4_black;
	Unigine::Math::vec4 bg_render_fade = Unigine::Math::vec4_zero;
	bool use_render_as_background = true;

	enum class GRAB_TYPE {
		NONE,
		PLANE,
		EDGE_LEFT,
		EDGE_RIGHT,
		EDGE_TOP,
		EDGE_BOTTOM,
		VERTEX_LT,
		VERTEX_RT,
		VERTEX_LB,
		VERTEX_RB
	};
	GRAB_TYPE mouse_grab_type = GRAB_TYPE::NONE;

	bool mouse_down = false;
	bool mouse_hold = false;
	bool mouse_up = false;
	Unigine::Math::ivec2 mouse_position;
	Unigine::Math::ivec2 mouse_down_pos;
	Unigine::Math::ivec2 mouse_hold_pos;

	struct Manipulator
	{
		Unigine::Math::vec2 p0;	   // left top corner
		Unigine::Math::vec2 p1;	   // left bottom corner
		Unigine::Math::vec2 p2;	   // right bottom corner
		Unigine::Math::vec2 p3;	   // right top corner
	} manipulator;				   // in window coordinates
	Unigine::Math::vec2 last_manipulator_p0;
	Unigine::Math::vec2 last_manipulator_p2;
	float last_manipulator_aspect;

	enum class AXIS { NONE, HORIZONTAL, VERTICAL };
	AXIS selected_axis = AXIS::NONE;
	const float select_axis_threshold = 10.0f;

	Unigine::Math::vec2 mouse_start_pos;
	Unigine::Vector<Unigine::Math::vec4> widget_canvas_start_pos;
	Unigine::Vector<Unigine::Math::ivec2> widget_screen_start_size;
	Unigine::Vector<float> widget_start_aspect;
	Unigine::Vector<Unigine::Math::vec4> widget_norm_pos;
	Unigine::Vector<Unigine::Math::vec4> widget_local_bounds;

	bool view_anchor = true;
	bool view_pivot = true;
	bool view_manipulator = true;

	const float vertex_radius = 6.0f;
	const float edge_thickness = 4.0f;
	const float anchor_size = 10.0f;

	bool use_snap_to_grid = true;
	int grid = 1;	 // current value
	int grid_size = 1;
	const int grid_multiplier = 10;	   // when using ctrl

	bool use_snap_to_elements = true;

	Unigine::Vector<UI::Element *> all_snappable_elements;
	void find_all_snappable_elements();
	float snap_x(float value);
	float snap_y(float value);
	Unigine::Math::vec2 snap(const Unigine::Math::vec2 &value);
	Unigine::Math::vec2 snap(float x, float y, float w, float h);	 // in canvas coords

	bool sync_selection = true;
	Unigine::Vector<Unigine::NodePtr> editor_selected_nodes;

	Unigine::WidgetDialogFilePtr file_dialog;
	Unigine::EventConnection file_dialog_ok;
	Unigine::EventConnection file_dialog_cancel;
	bool file_dialog_show = false;
	void show_file_dialog(const char *title, const char *filter);
	void close_file_dialog();

	// ui utils
	int get_position_x(const UI::ElementPtr &element) const;
	int get_position_y(const UI::ElementPtr &element) const;
	int get_width(const UI::ElementPtr &element) const;
	int get_height(const UI::ElementPtr &element) const;
	int get_screen_position_x(const UI::ElementPtr &element) const;
	int get_screen_position_y(const UI::ElementPtr &element) const;
	int get_screen_position_x1(const UI::ElementPtr &element) const;
	int get_screen_position_y1(const UI::ElementPtr &element) const;

	// editor
	class DesignerEditorLogic : public Unigine::EditorLogic
	{
	public:
		void setEnabled(bool enabled) { this->enabled = enabled; }
		bool isEnabled() const { return enabled; }

		void nodeReparented(const Unigine::NodePtr &node) override;
		void nodeReordered(const Unigine::NodePtr &node) override;
		void propertyChanged(const Unigine::UGUID &guid) override;

	private:
		bool enabled = true;
	};
	DesignerEditorLogic editor_logic;
};
