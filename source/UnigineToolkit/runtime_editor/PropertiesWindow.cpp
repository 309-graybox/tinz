// Copyright (C), UNIGINE. All rights reserved.
#include "PropertiesWindow.h"

#include "../imgui/imgui-node-editor/imgui_node_editor.h"
#include "../imgui/imgui/imgui.h"
#include "RuntimeEditor.h"

#include <UnigineCallback.h>
#include <UnigineComponentSystem.h>
#include <UnigineLogic.h>
#include <UnigineNode.h>
#include <UnigineProperties.h>
#include <UnigineVector.h>
#include <UnigineVisualizer.h>
#include <UnigineWorld.h>

using namespace Unigine;
using namespace Math;

namespace ed = ax::NodeEditor;

namespace {
struct NodeInfo
{
	NodePtr node;
	ed::NodeId id;	  // not the node->getID()!!!

	ed::PinId input_id;
	Vector<NodeInfo *> input_node_info;

	struct Output
	{
		ed::PinId id;
		StringStack<32> name;	 // PropertyName::ParameterName::ParameterChildName
		NodePtr node;
		NodeInfo *node_info;
	};
	VectorStack<Output, 6> output;

	bool show;		// if show_selected == true only
	bool sorted;	// for sort_selected() button
};
Vector<NodeInfo> nodes;
int unique_node_id = 1;

struct LinkInfo
{
	ed::LinkId id;
	ed::PinId input_id;
	ed::PinId output_id;
};
Vector<LinkInfo> links;	   // List of live links. It is dynamic unless you want to create read-only
						   // view over nodes.
int unique_link_id = 1;

ed::EditorContext *g_Context = nullptr;	   // Editor context, required to trace a editor state.
bool g_FirstFrame = true;	 // Flag set for first frame only, some action need to be executed once.
bool g_IsInit = false;		 // Flag for delayed init

// settings
NodePtr selection_node;
bool sort_and_fit_next_frame = false;
bool show_all_next_frame = false;

char filter_str[128] = "";
bool hide_empty = true;
bool show_selected = true;
bool auto_sort = true;

bool use_visualizer = true;
Vector<Pair<Vec3 /*3D pos*/, StringStack<32> /*name*/>> visualizer_node_names;

void ImGuiEx_BeginColumn()
{
	ImGui::BeginGroup();
}

void ImGuiEx_NextColumn()
{
	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::BeginGroup();
}

void ImGuiEx_EndColumn()
{
	ImGui::EndGroup();
}

bool has_node_parameters(const PropertyParameterPtr &prop)
{
	for (int i = 0; i < prop->getNumChildren(); i++)
	{
		PropertyParameterPtr p = prop->getChild(i);
		if (p->getType() == Property::PARAMETER_NODE)
			return true;
		if (p->getNumChildren() && has_node_parameters(p))
			return true;
	}
	return false;
}

void add_node_parameters(const PropertyParameterPtr &prop, Vector<NodeInfo::Output> &output,
	int &id, const char *prefix = nullptr)
{
	for (int i = 0; i < prop->getNumChildren(); i++)
	{
		PropertyParameterPtr p = prop->getChild(i);
		if (p->getType() == Property::PARAMETER_NODE)
		{
			NodeInfo::Output &o = output.append();
			o.id = id++;
			o.name = StringStack<>(prefix != nullptr
									   ? String::format("%s::%s", prefix, p->getName()).get()
									   : p->getName());
			o.node = p->getValueNode();
			o.node_info = nullptr;
		}
		if (p->getNumChildren() && has_node_parameters(p))
		{
			StringStack<> prefix_str = StringStack<>(
				prefix != nullptr ? String::format("%s::%s", prefix, p->getName()).get()
								  : p->getName());
			add_node_parameters(p, output, id, prefix_str);
		}
	}
}

void create_all_nodes()
{
	unique_node_id = 1;
	nodes.clear();

	Vector<NodePtr> world_nodes;
	World::getNodes(world_nodes);
	for (int i = 0; i < world_nodes.size(); i++)
	{
		NodePtr &node = world_nodes[i];
		if (!node->getNumProperties())
			continue;

		// find all properties that have parameters of type "PARAMETER_NODE"
		VectorStack<PropertyPtr, 6> properties_with_node_links;
		for (int j = 0; j < node->getNumProperties(); j++)
		{
			PropertyPtr prop = node->getProperty(j);
			if (!prop)
				continue;
			PropertyParameterPtr p = prop->getParameterPtr();
			if (!has_node_parameters(p))
				continue;

			// success!
			properties_with_node_links.append(prop);
		}

		// fill the node with info
		if (!properties_with_node_links.size())
			continue;
		NodeInfo &info = nodes.append();
		info.node = node;
		info.id = unique_node_id++;
		info.input_id = unique_node_id++;
		for (int j = 0; j < properties_with_node_links.size(); j++)
		{
			PropertyPtr &prop = properties_with_node_links[j];
			add_node_parameters(
				prop->getParameterPtr(), info.output, unique_node_id, prop->getName());
		}
	}
}

bool find_node_input_id(const Vector<NodeInfo> &in_nodes, const NodePtr &node, ed::PinId &out_id)
{
	for (int i = 0; i < in_nodes.size(); i++)
		if (node == in_nodes[i].node)
		{
			out_id = in_nodes[i].input_id;
			return true;
		}
	return false;
}

void create_all_links()
{
	unique_link_id = 1;
	links.clear();

	Vector<NodeInfo> add_nodes;

	for (int i = 0; i < nodes.size(); i++)
	{
		NodeInfo &info = nodes[i];
		for (int j = 0; j < info.output.size(); j++)
		{
			NodeInfo::Output &o = info.output[j];
			if (!o.node)
				continue;

			LinkInfo &link = links.append();
			link.id = unique_link_id++;
			link.input_id = o.id;
			if (!find_node_input_id(nodes, o.node, link.output_id)
				&& !find_node_input_id(add_nodes, o.node, link.output_id))
			{
				// need to create new node
				NodeInfo &new_info = add_nodes.append();
				new_info.node = o.node;
				new_info.id = unique_node_id++;
				new_info.input_id = unique_node_id++;
				link.output_id = new_info.input_id;
			}
		}
	}

	// add additional nodes without properties
	nodes.append(add_nodes);

	// fill NodeInfo::input_node_info and NodeInfo::Output::node_info
	for (int i = 0; i < nodes.size(); i++)
	{
		NodeInfo &info = nodes[i];
		for (int j = 0; j < info.output.size(); j++)
		{
			NodeInfo::Output &o = info.output[j];
			if (!o.node)
				continue;

			for (int k = 0; k < nodes.size(); k++)
			{
				NodeInfo &info2 = nodes[k];
				if (o.node == info2.node)
				{
					o.node_info = &info2;
					info2.input_node_info.append(&info);
					break;
				}
			}
		}
	}
}

void draw_node(const NodeInfo &info)
{
	ed::BeginNode(info.id);
	ImGui::Text("%s", info.node->getName());
	ImGuiEx_BeginColumn();
	ed::BeginPin(info.input_id, ed::PinKind::Input);
	ImGui::Text("->");
	ed::EndPin();
	ImGuiEx_NextColumn();

	float width = 0;
	for (int j = 0; j < info.output.size(); j++)
	{
		const NodeInfo::Output &o = info.output[j];
		if (hide_empty && (!o.node || o.node == info.node))
			continue;

		StringStack<> text = String::format("%s", o.name.get());
		width = Math::max(width, ImGui::CalcTextSize(text.get()).x);
	}

	for (int j = 0; j < info.output.size(); j++)
	{
		const NodeInfo::Output &o = info.output[j];
		if (hide_empty && (!o.node || o.node == info.node))
			continue;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - ImGui::CalcTextSize(o.name.get()).x);
		ImGui::TextUnformatted(o.name.get());
		ImGui::SameLine();
		ed::BeginPin(o.id, ed::PinKind::Output);
		ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
		ed::PinPivotSize(ImVec2(0, 0));
		ImGui::TextUnformatted("->");
		ed::EndPin();
	}
	ImGuiEx_EndColumn();
	ed::EndNode();

	if (use_visualizer && show_selected)
		visualizer_node_names.append(
			Pair<Vec3, StringStack<32>>(info.node->getWorldPosition(), info.node->getName()));
}

void draw_all_nodes()
{
	StringStack<> filter_l = filter_str;
	filter_l.lower();

	for (int i = 0; i < nodes.size(); i++)
	{
		NodeInfo &info = nodes[i];

		if (g_FirstFrame)
		{
			vec3 pos = vec3(info.node->getWorldPosition());
			ed::SetNodePosition(info.id, ImVec2(pos.x * 10, pos.y * 10));
		}

		// use filter
		if (!filter_l.empty())
		{
			StringStack<> node_name = info.node->getName();
			node_name.lower();
			if (!node_name.contains(filter_l))
				continue;	 // don't show this node
		}

		draw_node(info);
	}
}

void find_input_links(NodeInfo &info)
{
	for (int i = 0; i < info.input_node_info.size(); i++)
	{
		NodeInfo *input_info = info.input_node_info[i];
		if (input_info->show)
			continue;

		input_info->show = true;
		find_input_links(*input_info);

		if (use_visualizer)
			Visualizer::renderLine3D(info.node->getWorldPosition(),
				input_info->node->getWorldPosition(), vec4_red, 0, false);
	}
}

void find_output_links(NodeInfo &info)
{
	for (int j = 0; j < info.output.size(); j++)
	{
		const NodeInfo::Output &o = info.output[j];
		if (!o.node)
			continue;

		NodeInfo *output_info = o.node_info;
		if (output_info->show)
			continue;

		output_info->show = true;
		find_output_links(*output_info);

		if (use_visualizer)
			Visualizer::renderLine3D(info.node->getWorldPosition(),
				output_info->node->getWorldPosition(), vec4_green, 0, false);
	}
}

void draw_selected_nodes()
{
	VectorStack<ed::NodeId> selectedNodes;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	int nodeCount =
		ed::GetSelectedNodes(selectedNodes.get(), static_cast<int>(selectedNodes.size()));
	selectedNodes.resize(nodeCount);
	if (nodeCount == 0)	   // selected links only, not nodes :(
		return;

	VectorStack<int> selected_node_index;
	for (int i = 0; i < nodes.size(); i++)
	{
		NodeInfo &info = nodes[i];
		info.show = false;

		for (int j = 0; j < selectedNodes.size(); j++)
			if (selectedNodes[j] == info.id)
			{
				info.show = true;
				selected_node_index.append(i);
				break;
			}
	}

	// find all links related to selected nodes
	for (int i = 0; i < selected_node_index.size(); i++)
	{
		NodeInfo &info = nodes[selected_node_index[i]];
		find_input_links(info);
		find_output_links(info);
	}

	// show nodes
	for (int i = 0; i < nodes.size(); i++)
	{
		NodeInfo &info = nodes[i];
		if (!info.show)
			continue;

		if (g_FirstFrame)
		{
			vec3 pos = vec3(info.node->getWorldPosition());
			ed::SetNodePosition(info.id, ImVec2(pos.x * 10, pos.y * 10));
		}

		draw_node(info);
	}
}

void draw_all_links()
{
	for (auto &info : links)
		ed::Link(info.id, info.input_id, info.output_id);
}

void refresh()
{
	g_IsInit = true;
	g_FirstFrame = true;
	create_all_nodes();
	create_all_links();
	show_all_next_frame = true;
}

void push_inputs(NodeInfo &info, const ImVec2 &pos, const ImVec2 &size)
{
	if (!info.input_node_info.size())
		return;

	float y_offset = 0;
	for (int i = 0; i < info.input_node_info.size(); i++)
	{
		NodeInfo &left = *info.input_node_info[i];
		if (left.sorted)
			continue;
		left.sorted = true;

		ImVec2 new_size = ed::GetNodeSize(left.id);

		ImVec2 new_pos = pos;
		new_pos.x -= new_size.x + 100;
		new_pos.y += y_offset;
		ed::SetNodePosition(left.id, new_pos);

		y_offset += new_size.y + 5;

		push_inputs(left, new_pos, new_size);
	}
}

float push_outputs(NodeInfo &info, const ImVec2 &pos, const ImVec2 &size)
{
	if (!info.output.size())
		return 0;

	float y_offset = 0;
	for (int i = 0; i < info.output.size(); i++)
	{
		NodeInfo *right_node = info.output[i].node_info;
		if (!right_node || right_node->sorted)
			continue;
		NodeInfo &right = *right_node;
		right.sorted = true;

		ImVec2 new_size = ed::GetNodeSize(right.id);

		ImVec2 new_pos = pos;
		new_pos.x += size.x + 100;
		new_pos.y += y_offset;
		ed::SetNodePosition(right.id, new_pos);

		float child_offset = push_outputs(right, new_pos, new_size);
		if (child_offset == 0)
			y_offset += new_size.y + 5;
		else
			y_offset += child_offset;
	}
	return y_offset;
}

void sort_selected()
{
	VectorStack<ed::NodeId> selected_nodes_id;
	selected_nodes_id.resize(ed::GetSelectedObjectCount());
	int nodeCount =
		ed::GetSelectedNodes(selected_nodes_id.get(), static_cast<int>(selected_nodes_id.size()));
	selected_nodes_id.resize(nodeCount);
	if (nodeCount == 0)	   // selected links only, not nodes :(
		return;

	// reset "sorted" flags
	VectorStack<int> selected_nodes;
	for (int i = 0; i < nodes.size(); i++)
	{
		NodeInfo &info = nodes[i];
		if (info.show)
		{
			info.sorted = false;

			if (selected_nodes_id.findIndex(info.id) != -1)
			{
				selected_nodes.append(i);
				info.sorted = true;
			}
		}
	}

	// calculate and apply new positions
	for (int i = 0; i < selected_nodes.size(); i++)
	{
		NodeInfo &info = nodes[selected_nodes[i]];
		ImVec2 pos = ed::GetNodePosition(info.id);
		ImVec2 size = ed::GetNodeSize(info.id);
		push_inputs(info, pos, size);
		push_outputs(info, pos, size);
	}
}
}	 // namespace

void PropertiesWindow::init()
{
	ed::Config config;
	config.SettingsFile = "PropertiesNodeEditor.json";
	g_Context = ed::CreateEditor(&config);
}

void PropertiesWindow::render(bool *p_open)
{
	if (!g_IsInit)
		refresh();

	ImGui::Begin("Properties", p_open);

	ImGui::BeginChild("Properties", ImVec2(0, 85), true);
	ImGui::InputText("Filter", filter_str, IM_ARRAYSIZE(filter_str));
	ImGui::SameLine();
	if (ImGui::Button("X"))
		filter_str[0] = '\0';
	ImGui::Text("Nodes: %d", nodes.size());
	ImGui::SameLine();
	// ImGui::Checkbox("Hide empty output links", &hide_empty);
	ImGui::Checkbox("Show selected nodes only", &show_selected);
	if (show_selected)
	{
		ImGui::SameLine();
		ImGui::Checkbox("Use Visualizer", &use_visualizer);
		ImGui::SameLine();
		ImGui::Checkbox("Auto sort nodes", &auto_sort);
	}
	if (ImGui::Button("Refresh"))
		refresh();
	ImGui::SameLine();
	bool sort = ImGui::Button("Sort Selected");
	ImGui::SameLine();
	bool fit_selected = ImGui::Button("Fit Selected");
	ImGui::SameLine();
	bool fit_view = ImGui::Button("Fit View");
	ImGui::EndChild();

	ed::SetCurrentEditor(g_Context);
	ed::EnableShortcuts(false);

	// Start interaction with editor.
	ed::Begin("My Editor", ImVec2(0.0, 0.0f));

	visualizer_node_names.clear();
	if (show_selected && !show_all_next_frame)
		draw_selected_nodes();
	else
		draw_all_nodes();
	draw_all_links();
	show_all_next_frame = false;

	// show nodes in the world
	if (use_visualizer)
	{
		// merge names in the same position
		for (int i = 0; i < visualizer_node_names.size(); i++)
		{
			auto &name_i = visualizer_node_names[i];
			for (int j = i + 1; j < visualizer_node_names.size(); j++)
			{
				auto &name_j = visualizer_node_names[j];
				if (name_i.first == name_j.first)
				{
					name_i.second += String::format(",\n%s", name_j.second.get());
					visualizer_node_names.removeFast(j);
					j--;
				}
			}
		}

		// show names
		for (int i = 0; i < visualizer_node_names.size(); i++)
		{
			auto &name_i = visualizer_node_names[i];
			Visualizer::renderMessage3D(name_i.first, vec3_zero, name_i.second, vec4_white, 1);
		}
	}

	// change selection from outside
	NodePtr prev_selection_node = selection_node;
	selection_node = RuntimeEditor::getSelectedNode();
	if (selection_node != prev_selection_node)
	{
		if (selection_node)
		{
			bool selected = false;
			for (int i = 0; i < nodes.size(); i++)
			{
				NodeInfo &info = nodes[i];
				if (info.node == selection_node)
				{
					ed::SelectNode(info.id);
					selected = true;
					break;
				}
			}
			if (!selected)
			{
				// try ed::SelectNode in next frame
				refresh();
				ed::ClearSelection();
			}
			if (auto_sort)
				sort = true;
			fit_view = true;
			sort_and_fit_next_frame = true;
		}
		else
			ed::ClearSelection();
	}
	else if (sort_and_fit_next_frame)
	{
		if (auto_sort)
			sort = true;
		fit_view = true;
		sort_and_fit_next_frame = false;
	}

	ed::End();

	if (sort)
		sort_selected();

	if (g_FirstFrame || fit_view)
		ed::NavigateToContent(0.5f);
	if (fit_selected)
		ed::NavigateToSelection(true, 0.5f);

	ed::SetCurrentEditor(nullptr);

	g_FirstFrame = false;

	ImGui::End();
}

void PropertiesWindow::shutdown()
{
	ed::DestroyEditor(g_Context);
	g_IsInit = false;
}
