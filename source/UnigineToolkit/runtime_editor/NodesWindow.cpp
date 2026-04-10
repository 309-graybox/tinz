// Copyright (C), UNIGINE. All rights reserved.
#include "NodesWindow.h"

#include "../imgui/imgui/imgui.h"
#include "IShowInRuntimeEditor.h"
#include "RuntimeEditor.h"

#include <UnigineCallback.h>
#include <UnigineComponentSystem.h>
#include <UnigineLogic.h>
#include <UnigineNode.h>
#include <UnigineVector.h>
#include <UnigineWorld.h>

using namespace Unigine;
using namespace Math;

namespace {
Vector<NodePtr> nodes;

// doesn't used yet:
class NodesEditorLogic : public EditorLogic
{
public:
	void nodeReparented(const NodePtr &node) override {}
	void nodeReordered(const NodePtr &node) override {}
	void nodeRenamed(const NodePtr &node, const char *old_name) override {}
};
void onCacheNodeAdd(const NodePtr &node)
{}
void onNodeLoad(const NodePtr &node)
{}
void onNodeClone(const NodePtr &node)
{}
void onNodeSwap(const NodePtr &node)
{}
void onNodeRemove(const NodePtr &node)
{}
void onNodeChangeEnabled(const NodePtr &node)
{}
Unigine::EventConnections event_connections;

int selected_node_id = -1;
Vector<NodePtr> selected_parents;

int focused_node_id = -1;
// Sometimes tree item may not become visible from ImGui standpoint;
// Therefore, we use the focus attempts counter to avoid falling into an endless
// ImGui::SetScrollHereY() loop.
constexpr int max_node_focus_attempts = 8;
int node_focus_attempts_count = 0;
void set_focused_node_id(int node_id)
{
	if (focused_node_id == node_id)
		return;

	focused_node_id = node_id;
	node_focus_attempts_count = 0;
}

void select_node(const NodePtr &node)
{
	if (node)
	{
		RuntimeEditor::selectNode(node);
		selected_node_id = node->getID();
	}
	else
	{
		RuntimeEditor::clearNodeSelection();
		selected_node_id = -1;
	}
}

bool filtered = false;
Unigine::Vector<NodePtr> filtered_nodes;
void filter_nodes(const char *filter)
{
	filtered = filter[0] != '\0';
	filtered_nodes.clear();
	if (!filtered)
		return;

	StringStack<> filter_l = filter;
	filter_l.lower();

	nodes.clear();
	World::getNodes(nodes);
	filtered_nodes.reserve(nodes.size() / 2);
	for (int i = 0; i < nodes.size(); i++)
	{
		NodePtr &node = nodes[i];
		if (StringStack<>(node->getName()).lower().contains(filter_l))
			filtered_nodes.push_back(node);
	}
}

bool draw_tree_node(const NodePtr &node, bool is_parent = false)
{
	static const ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
												 | ImGuiTreeNodeFlags_OpenOnDoubleClick
												 | ImGuiTreeNodeFlags_SpanAvailWidth;
	ImGuiTreeNodeFlags node_flags = base_flags;
	if (selected_node_id == node->getID())
		node_flags |= ImGuiTreeNodeFlags_Selected;
	if (!is_parent)
		node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	ImGui::PushStyleColor(
		ImGuiCol_Text, node->isEnabled() ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	StringStack<> name = node->getName();
	if (name.empty())
		name = String::format("[Unnamed %s]", node->getTypeName());
	bool node_open =
		ImGui::TreeNodeEx((void *)(intptr_t)node->getID(), node_flags, "%s", name.get());
	ImGui::PopStyleColor();
	if (ImGui::IsItemClicked())
		select_node(node);

	return node_open;
}

void draw_nodes_tree(const NodePtr &node)
{
	if (!node)
		return;

	if (selected_parents.size() && selected_parents.last() == node)
	{
		ImGui::SetNextItemOpen(selected_node_id != node->getID());
		selected_parents.removeLast();
	}

	const bool is_parent = node->getNumChildren() || node->getType() == Node::NODE_REFERENCE;
	bool node_open = draw_tree_node(node, is_parent);

	if (focused_node_id == node->getID())
	{
		if (!ImGui::IsItemVisible() && ++node_focus_attempts_count < max_node_focus_attempts)
			ImGui::SetScrollHereY();
		else
			focused_node_id = -1;
	}

	if (node_open && is_parent)
	{
		if (node->getType() == Node::NODE_REFERENCE)
		{
			NodePtr node_ref = static_ptr_cast<NodeReference>(node)->getReference();
			if (node_ref)
				draw_nodes_tree(node_ref);
		}

		for (int i = 0; i < node->getNumChildren(); i++)
		{
			draw_nodes_tree(node->getChild(i));
		}

		ImGui::TreePop();
	}
}

void draw_nodes_list()
{
	nodes.clear();
	if (filtered)
	{
		for (int i = 0; i < filtered_nodes.size(); i++)
		{
			NodePtr &node = filtered_nodes[i];
			if (node)
				draw_tree_node(node);
		}
	}
	else
	{
		World::getRootNodes(nodes);
		for (int i = 0; i < nodes.size(); i++)
			draw_nodes_tree(nodes[i]);
	}

	// ImGui does not scroll to the last tree item precisely;
	// Thus, we add an additional space at the bottom, that greatly mitigates such a behavior.
	ImVec2 size = ImGui::GetItemRectSize();
	ImGui::Dummy(size);
}

void draw_property_parameters(const PropertyParameterPtr &params)
{
	for (int i = 0; i < params->getNumChildren(); i++)
	{
		PropertyParameterPtr p = params->getChild(i);
		switch (p->getType())
		{
		case Property::PARAMETER_INT: {
			int value = p->getValueInt();
			if (ImGui::InputInt(p->getName(), &value))
				p->setValueInt(value);
		}
		break;
		case Property::PARAMETER_FLOAT: {
			float value = p->getValueFloat();
			if (ImGui::InputFloat(p->getName(), &value))
				p->setValueFloat(value);
		}
		break;
		case Property::PARAMETER_DOUBLE: {
			double value = p->getValueDouble();
			if (ImGui::InputDouble(p->getName(), &value))
				p->setValueDouble(value);
		}
		break;
		case Property::PARAMETER_TOGGLE: {
			bool value = p->getValueToggle();
			if (ImGui::Checkbox(p->getName(), &value))
				p->setValueToggle(value);
		}
		break;
		case Property::PARAMETER_SWITCH: {
			int value = p->getValueSwitch();
			StringStack<> items;
			for (int j = 0; j < p->getSwitchNumItems(); j++)
			{
				items += p->getSwitchItemName(j);
				items += '\0';
			}
			if (ImGui::Combo(p->getName(), &value, items))
				p->setValueSwitch(value);
		}
		break;
		case Property::PARAMETER_STRING: {
			char str[256] = "";
			memcpy(str, p->getValueString(), strlen(p->getValueString()) + 1 /*'\0'*/);
			if (ImGui::InputText(p->getName(), str, IM_ARRAYSIZE(str)))
				p->setValueString(str);
		}
		break;
		case Property::PARAMETER_COLOR: {
			vec4 value = p->getValueColor();
			if (ImGui::ColorEdit4(p->getName(), value))
				p->setValueColor(value);
		}
		break;
		case Property::PARAMETER_VEC2: {
			vec2 value = p->getValueVec2();
			if (ImGui::InputFloat2(p->getName(), value, "%.5f"))
				p->setValueVec2(value);
		}
		break;
		case Property::PARAMETER_VEC3: {
			vec3 value = p->getValueVec3();
			if (ImGui::InputFloat3(p->getName(), value, "%.5f"))
				p->setValueVec3(value);
		}
		break;
		case Property::PARAMETER_VEC4: {
			vec4 value = p->getValueVec4();
			if (ImGui::InputFloat4(p->getName(), value, "%.5f"))
				p->setValueVec4(value);
		}
		break;
		case Property::PARAMETER_DVEC2: {
			vec2 value = vec2(p->getValueDVec2());
			if (ImGui::InputFloat2(p->getName(), value, "%.5f"))
				p->setValueDVec2(dvec2(value));
		}
		break;
		case Property::PARAMETER_DVEC3: {
			vec3 value = vec3(p->getValueDVec3());
			if (ImGui::InputFloat3(p->getName(), value, "%.5f"))
				p->setValueDVec3(dvec3(value));
		}
		break;
		case Property::PARAMETER_DVEC4: {
			vec4 value = vec4(p->getValueDVec4());
			if (ImGui::InputFloat4(p->getName(), value, "%.5f"))
				p->setValueDVec4(dvec4(value));
		}
		break;
		case Property::PARAMETER_IVEC2: {
			ivec2 value = p->getValueIVec2();
			if (ImGui::InputInt2(p->getName(), value))
				p->setValueIVec2(value);
		}
		break;
		case Property::PARAMETER_IVEC3: {
			ivec3 value = p->getValueIVec3();
			if (ImGui::InputInt3(p->getName(), value))
				p->setValueIVec3(value);
		}
		break;
		case Property::PARAMETER_IVEC4: {
			ivec4 value = p->getValueIVec4();
			if (ImGui::InputInt4(p->getName(), value))
				p->setValueIVec4(value);
		}
		break;
		case Property::PARAMETER_MASK: {
			int value = p->getValueMask();
			if (ImGui::InputInt(p->getName(), &value))
				p->setValueMask(value);
		}
		break;
		case Property::PARAMETER_FILE: {
			char str[256] = "";
			memcpy(str, p->getValueFile(), strlen(p->getValueFile()) + 1 /*'\0'*/);
			if (ImGui::InputText(p->getName(), str, IM_ARRAYSIZE(str)))
				p->setValueFile(str);
		}
		break;
		case Property::PARAMETER_PROPERTY: {
			PropertyPtr value = p->getValueProperty();
			ImGui::Text("%s", value ? value->getName() : "(null)");
			ImGui::SameLine();
			ImGui::Text("%s", p->getName());
		}
		break;
		case Property::PARAMETER_MATERIAL: {
			MaterialPtr value = p->getValueMaterial();

			ImGui::Text(
				"%s", value ? FileSystem::getVirtualPath(value->getFilePath()).get() : "(null)");
			ImGui::SameLine();
			ImGui::Text("%s", p->getName());
		}
		break;
		case Property::PARAMETER_NODE: {
			NodePtr value = p->getValueNode();
			if (value)
			{
				if (ImGui::Button(
						value->getName(), ImVec2(ImGui::GetContentRegionAvail().x * 0.67f, 0.0f)))
					select_node(value);
			}
			else
			{
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.67f);
				ImGui::Text("(null)");
			}
			ImGui::SameLine();
			ImGui::Text("%s", p->getName());
		}
		break;
		case Property::PARAMETER_CURVE2D: {
			ImGui::Text("[Curve2D]");
			ImGui::SameLine();
			ImGui::Text("%s", p->getName());
		}
		break;
		case Property::PARAMETER_ARRAY: {
			if (ImGui::TreeNode(p->getName()))
			{
				draw_property_parameters(p);
				ImGui::TreePop();
			}
		}
		break;
		case Property::PARAMETER_STRUCT: {
			if (ImGui::TreeNode(p->getName()))
			{
				draw_property_parameters(p);
				ImGui::TreePop();
			}
		}
		break;
		}
	}
}

void draw_node_parameters()
{
	if (selected_node_id == -1)
		return;
	NodePtr node = World::getNodeByID(selected_node_id);
	if (!node)
		return;

	// - common ---------------------------------------------

	bool enabled = node->isEnabled();
	if (ImGui::Checkbox("Enabled", &enabled))
		node->setEnabled(enabled);

	ImGui::Text("ID: %d", node->getID());
	ImGui::Text("Name: %s", node->getName());
	ImGui::Text("Type: %s", node->getTypeName());

	const char *items[] = {"XYZ (default)", "XZY", "YXZ (camera)", "YZX", "ZXY (object)", "ZYX"};
	static int item_current = 0;
	ImGui::Combo("Euler order", &item_current, items, IM_ARRAYSIZE(items));
	Mat4 t = node->getTransform();
	Vec3 pos = t.getTranslate();
	vec3 rot;
	switch (item_current)
	{
	case 0:
		rot = decomposeRotationXYZ(mat3(t));
		break;
	case 1:
		rot = decomposeRotationXZY(mat3(t));
		break;
	case 2:
		rot = decomposeRotationYXZ(mat3(t));
		break;
	case 3:
		rot = decomposeRotationYZX(mat3(t));
		break;
	case 4:
		rot = decomposeRotationZXY(mat3(t));
		break;
	case 5:
		rot = decomposeRotationZYX(mat3(t));
		break;
	}
	vec3 s = t.getScale();
	vec3 pos_float = vec3(pos);
	bool changed = ImGui::InputFloat3("Position", pos_float, "%.5f");
	if (changed)
		pos = Vec3(pos_float);
	changed |= ImGui::InputFloat3("Rotation", rot, "%.5f");
	changed |= ImGui::InputFloat3("Scale", s, "%.5f");
	if (changed)
	{
		mat4 rot_mat;
		switch (item_current)
		{
		case 0:
			rot_mat = composeRotationXYZ(rot);
			break;
		case 1:
			rot_mat = composeRotationXZY(rot);
			break;
		case 2:
			rot_mat = composeRotationYXZ(rot);
			break;
		case 3:
			rot_mat = composeRotationYZX(rot);
			break;
		case 4:
			rot_mat = composeRotationZXY(rot);
			break;
		case 5:
			rot_mat = composeRotationZYX(rot);
			break;
		}
		node->setTransform(translate(pos) * Mat4(rot_mat) * scale(Vec3(s)));
	}

	// - light -----------------------------------------

	LightPtr light = checked_ptr_cast<Light>(node);
	if (light)
	{
		ImGui::Spacing();
		ImGui::SeparatorText("Light");
		ImGui::Text("Viewport Mask: %#x", light->getViewportMask());
		ImGui::Text("Shadow Mask: %#x", light->getShadowMask());

		if (ImGui::TreeNodeEx("Visible Distance", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Light %g", light->getVisibleDistance());
			ImGui::Text("Light Fade %g", light->getFadeDistance());
			ImGui::Text("Shadow %g", light->getShadowDistance());
			ImGui::Text("Shadow Fade %g", light->getShadowFadeDistance());
			ImGui::TreePop();
		}

		// Light::MODE getMode();

		if (ImGui::TreeNodeEx("Shadow", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SameLine();
			bool shadow = light->getShadow();
			if (ImGui::Checkbox("##Shadow", &shadow))
				light->setShadow(shadow);

			auto shadow_mode_to_str = [](Light::SHADOW_MODE v) {
				const char *strings[]{"Mixed", "Static"};
				if (v >= Light::SHADOW_MODE_MIXED && v <= Light::SHADOW_MODE_STATIC)
					return strings[v];
				return "?";
			};
			auto shadow_res_to_str = [](Light::SHADOW_RESOLUTION v) {
				const char *strings[]{
					"default",	  //-1
					"64",
					"128",
					"256",
					"512",
					"1024",
					"2048",
					"4096",
					"8192",
					"16384",
				};
				if (v >= Light::SHADOW_RESOLUTION_MODE_DEFAULT
					&& v <= Light::SHADOW_RESOLUTION_MODE_16384)
					return strings[v + 1];
				return "?";
			};
			ImGui::Text("Mode %s", shadow_mode_to_str(light->getShadowMode()));
			ImGui::Text("Resolution %s", shadow_res_to_str(light->getShadowResolution()));

			float bias = light->getShadowBias();
			if (ImGui::InputFloat("Bias", &bias))
				light->setShadowBias(bias);
			float normal_bias = light->getShadowNormalBias();
			if (ImGui::InputFloat("Normal Bias", &normal_bias))
				light->setShadowNormalBias(normal_bias);

			bool ss_shadow = light->isShadowScreenSpace();
			if (ImGui::Checkbox("Screen Space Shadow", &ss_shadow))
				light->setShadowScreenSpace(ss_shadow);

			ImGui::TreePop();
		}
		ImGui::Separator();
	}

	// - properties -----------------------------------------

	ImGui::Spacing();
	ImGui::Text("Properties: %d", node->getNumProperties());
	for (int i = 0; i < node->getNumProperties(); i++)
	{
		PropertyPtr prop = node->getProperty(i);
		if (!prop)
			continue;

		if (ImGui::CollapsingHeader(
				String::format("%s##_prop_%d", prop->getName(), i), ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushID(i);
			bool property_enabled = node->isPropertyEnabled(i);
			if (ImGui::Checkbox("Property Enabled", &property_enabled))
				node->setPropertyEnabled(i, property_enabled);
			ImGui::Spacing();

			// parameters
			PropertyParameterPtr params = prop->getParameterPtr();
			draw_property_parameters(params);

			// custom UI
			VectorStack<IShowInRuntimeEditor *> components;
			ComponentSystem::get()->getComponents<IShowInRuntimeEditor>(node, components);
			if (components.size())
			{
				for (int j = 0; j < components.size(); j++)
					components[j]->editorUpdate();
			}
			ImGui::PopID();
		}
	}
}

static int selected_shape = -1;
static int selected_joint = -1;
void draw_physics_parameters()
{
	if (selected_node_id == -1)
		return;
	NodePtr node = World::getNodeByID(selected_node_id);
	if (!node)
		return;
	ObjectPtr obj = checked_ptr_cast<Object>(node);
	if (!obj)
		return;

	BodyPtr body = obj->getBody();
	if (!body)
	{
		ImGui::Text("Body not set");
		return;
	}

	ImGui::Text("Body type: %s", body->getTypeName());
	ImGui::Separator();
	ImGui::Text("Flags");
	ImGui::Separator();

	bool is_enabled = body->isEnabled();
	if (ImGui::Checkbox("Enabled##nw", &is_enabled))
		body->setEnabled(is_enabled);

	auto rb = checked_ptr_cast<BodyRigid>(body);
	if (rb)
	{
		ImGui::Text("Is Freezable: %s", rb->isFreezable() ? "yes" : "no");
		ImGui::Text("Is Frozen: %s", rb->isFrozen() ? "yes" : "no");
	}

	if (rb)
	{
		float linear_damping = rb->getLinearDamping();
		if (ImGui::InputFloat("Linear Damping", &linear_damping))
			rb->setLinearDamping(linear_damping);

		float angular_damping = rb->getAngularDamping();
		if (ImGui::InputFloat("Angular Damping", &angular_damping))
			rb->setAngularDamping(angular_damping);

		float max_linear_velocity = rb->getMaxLinearVelocity();
		if (ImGui::InputFloat("Max Linear Velocity", &max_linear_velocity))
			rb->setMaxLinearVelocity(max_linear_velocity);

		float max_angular_velocity = rb->getMaxAngularVelocity();
		if (ImGui::InputFloat("Max Angular Velocity", &max_angular_velocity))
			rb->setMaxAngularVelocity(max_angular_velocity);

		float frozen_linear_velocity = rb->getFrozenLinearVelocity();
		if (ImGui::InputFloat("Frozen Linear Velocity", &frozen_linear_velocity))
			rb->setFrozenLinearVelocity(frozen_linear_velocity);

		float frozen_angular_velocity = rb->getFrozenAngularVelocity();
		if (ImGui::InputFloat("Frozen Angular Velocity", &frozen_angular_velocity))
			rb->setFrozenAngularVelocity(frozen_angular_velocity);
	}

	// ----------------------------------------------------------------------------

	ImGui::Separator();
	ImGui::Text("Shapes: %d", body->getNumShapes());
	ImGui::Separator();
	static bool show_all_shapes = false;
	ImGui::Checkbox("Show all shapes", &show_all_shapes);

	ImGui::BeginTable(String::format("Shapes: %d", body->getNumShapes()), 1,
		ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders);
	for (int i = 0; i < body->getNumShapes(); i++)
	{
		ImGui::TableNextColumn();

		ImGui::PushID(i);
		auto shape = body->getShape(i);
		bool shape_enabled = shape->isEnabled();
		if (ImGui::Checkbox("##SHAPE_ENABLED", &shape_enabled))
			shape->setEnabled(shape_enabled);

		ImGui::SameLine();
		ImGui::Selectable(String::format("%s %s", shape->getTypeName(), shape->getName()).get(),
			selected_shape == i);

		if (ImGui::IsItemClicked())
			selected_shape = selected_shape == i ? selected_shape = -1 : i;

		if (body->getShape(i)->isEnabled() && (show_all_shapes || selected_shape == i))
			body->getShape(i)->renderVisualizer(vec4_blue);

		ImGui::PopID();
	}
	ImGui::EndTable();

	// ----------------------------------------------------------------------------

	ImGui::Separator();
	ImGui::Text("Joints: %d", body->getNumJoints());
	ImGui::Separator();
	static bool show_all_joints = false;
	ImGui::Checkbox("Show all joints", &show_all_joints);

	ImGui::BeginTable(String::format("Joints: %d", body->getNumJoints()), 1,
		ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders);
	for (int i = 0; i < body->getNumJoints(); i++)
	{
		ImGui::TableNextColumn();

		auto joint = body->getJoint(i);
		bool joint_enabled = joint->isEnabled();
		if (ImGui::Checkbox(String::format("##JOINT_ENABLED_%d", i).get(), &joint_enabled))
			joint->setEnabled(joint_enabled);

		ImGui::SameLine();
		ImGui::Selectable(String::format("%s %s", joint->getTypeName(), joint->getName()).get(),
			selected_joint == i);

		if (ImGui::IsItemClicked())
			selected_joint = selected_joint == i ? selected_joint = -1 : i;

		if (joint->isEnabled() && (show_all_joints || selected_joint == i))
			joint->renderVisualizer(vec4_blue);
	}
	ImGui::EndTable();

	if (selected_joint != -1)
	{
		auto joint = body->getJoint(selected_joint);

		int iterations = joint->getNumIterations();
		if (ImGui::InputInt("Iterations", &iterations))
			joint->setNumIterations(iterations);

		float max_force = joint->getMaxForce();
		if (ImGui::InputFloat("Max Force", &max_force))
			joint->setMaxForce(max_force);

		float max_torque = joint->getMaxTorque();
		if (ImGui::InputFloat("Max Torque", &max_torque))
			joint->setMaxTorque(max_torque);

		float l_restitution = joint->getLinearRestitution();
		if (ImGui::InputFloat("Linear Restitution", &l_restitution))
			joint->setLinearRestitution(l_restitution);

		float a_restitution = joint->getAngularRestitution();
		if (ImGui::InputFloat("Angular Restitution", &a_restitution))
			joint->setAngularRestitution(a_restitution);

		float l_softness = joint->getLinearSoftness();
		if (ImGui::InputFloat("Linear Softness", &l_softness))
			joint->setLinearSoftness(l_softness);

		float a_softness = joint->getAngularSoftness();
		if (ImGui::InputFloat("Angular Softness", &a_softness))
			joint->setAngularSoftness(a_softness);

		if (joint->getType() == Joint::JOINT_WHEEL)
		{
			auto joint_wheel = static_ptr_cast<JointWheel>(joint);

			float linear_damping = joint_wheel->getLinearDamping();
			if (ImGui::InputFloat("Linear Damping", &linear_damping))
				joint_wheel->setLinearDamping(linear_damping);

			float linear_distance = joint_wheel->getLinearDistance();
			if (ImGui::InputFloat("Linear Distance", &linear_distance))
				joint_wheel->setLinearDistance(linear_distance);

			float l_limit_from = joint_wheel->getLinearLimitFrom();
			if (ImGui::InputFloat("Linear Limit From", &l_limit_from))
				joint_wheel->setLinearLimitFrom(l_limit_from);

			float l_limit_to = joint_wheel->getLinearLimitTo();
			if (ImGui::InputFloat("Linear Limit To", &l_limit_to))
				joint_wheel->setLinearLimitTo(l_limit_to);

			float linear_spring = joint_wheel->getLinearSpring();
			if (ImGui::InputFloat("Linear Spring", &linear_spring))
				joint_wheel->setLinearSpring(linear_spring);
		}
	}
}
void draw_masks()
{
	if (selected_node_id == -1)
		return;
	NodePtr node = World::getNodeByID(selected_node_id);
	if (!node)
		return;
	ObjectPtr obj = checked_ptr_cast<Object>(node);
	if (!obj)
	{
		ImGui::Text("Not a object");
		return;
	}
	ImGui::Text("Surfaces num: %d", obj->getNumSurfaces());

	constexpr int num_surfaces_draw_limit = 100;
	if (ImGui::CollapsingHeader("Surfaces viewport masks"))
	{
		for (int i = 0; i < min(obj->getNumSurfaces(), num_surfaces_draw_limit); i++)
			ImGui::Text("Surface %s: mask: %#x", obj->getSurfaceName(i), obj->getViewportMask(i));
	}

	if (ImGui::CollapsingHeader("Surfaces collision masks"))
	{
		for (int i = 0; i < min(obj->getNumSurfaces(), num_surfaces_draw_limit); i++)
			ImGui::Text("Surface %s: mask: %#x", obj->getSurfaceName(i), obj->getCollisionMask(i));
	}

	if (ImGui::CollapsingHeader("Surfaces phys intersection masks"))
	{
		for (int i = 0; i < min(obj->getNumSurfaces(), num_surfaces_draw_limit); i++)
			ImGui::Text("Surface %s: mask: %#x", obj->getSurfaceName(i),
				obj->getPhysicsIntersectionMask(i));
	}

	if (ImGui::CollapsingHeader("Surfaces intersection masks"))
	{
		for (int i = 0; i < min(obj->getNumSurfaces(), num_surfaces_draw_limit); i++)
			ImGui::Text(
				"Surface %s: mask: %#x", obj->getSurfaceName(i), obj->getIntersectionMask(i));
	}
}

static char search_text[128] = "";
int search_timer_id = -1;
}	 // namespace

void NodesWindow::init()
{
	Node::getEventCacheNodeAdd().connect(event_connections, onCacheNodeAdd);
	Node::getEventNodeLoad().connect(event_connections, onNodeLoad);
	Node::getEventNodeClone().connect(event_connections, onNodeClone);
	Node::getEventNodeSwap().connect(event_connections, onNodeSwap);
	Node::getEventNodeRemove().connect(event_connections, onNodeRemove);
	Node::getEventNodeChangeEnabled().connect(event_connections, onNodeChangeEnabled);

	search_timer_id = RuntimeEditor::addUiTimer();
}

void NodesWindow::render(bool *p_open)
{
	if (Input::isKeyPressed(Input::KEY_ESC))
	{
		RuntimeEditor::clearNodeSelection();
		selected_node_id = -1;
		focused_node_id = -1;
	}
	else if (RuntimeEditor::isSelectedNode())
	{
		int prev_node_selected = selected_node_id;
		selected_node_id = RuntimeEditor::getSelectedNode()->getID();
		if (selected_node_id != prev_node_selected)
		{
			selected_shape = -1;
			selected_joint = -1;

			set_focused_node_id(selected_node_id);
			NodePtr node = RuntimeEditor::getSelectedNode();
			selected_parents.clear();
			while (node)
			{
				selected_parents.append(node);
				NodePtr parent = node->getParent();
				node = parent ? parent : node->getPossessor();
			}
		}
	}
	else
		selected_node_id = -1;

	ImGui::Begin("Nodes", p_open);

	if (ImGui::BeginTable("NodesParameters", 2,
			ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV
				| ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupScrollFreeze(0, 1);	// Make top row always visible
		ImGui::TableSetupColumn("Nodes", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();

		// tree search and nodes controls bar
		bool update_search = false;
		ImGui::TableSetColumnIndex(0);
		ImGui::BeginChild("Filters", ImVec2(0, 60), true);
		if (ImGui::InputText("Filter", search_text, IM_ARRAYSIZE(search_text)))
			RuntimeEditor::setUiTimeout(search_timer_id, .2);
		ImGui::SameLine();
		if (ImGui::Button("X"))
		{
			search_text[0] = '\0';
			update_search = true;
		}
		ImGui::Spacing();	 // next line
		if (ImGui::Button("Refresh"))
			update_search = true;
		ImGui::SameLine();
		ImGui::Button("Collapse All");
		ImGui::SameLine();
		ImGui::Button("Expand All");
		ImGui::EndChild();

		// nodes tree view
		ImGui::BeginChild("ChildNodes", ImVec2(0, 0), false,
			ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysVerticalScrollbar);
		if (update_search || RuntimeEditor::checkUiTimeout(search_timer_id))
		{
			RuntimeEditor::clearUiTimeout(search_timer_id);

			const bool was_filtered = filtered;
			filter_nodes(search_text);
			if (was_filtered && !filtered)
				selected_node_id = -1;	  // Trigger selected parents update
		}
		draw_nodes_list();
		ImGui::EndChild();

		// parameters
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginChild("ChildParameters", ImVec2(0, 0), true);

		if (ImGui::BeginTabBar("NodesPhysics", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Node"))
			{
				draw_node_parameters();
				ImGui::EndTabItem();
				selected_shape = -1;
				selected_joint = -1;
			}
			if (ImGui::BeginTabItem("Physics"))
			{
				draw_physics_parameters();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Masks"))
			{
				draw_masks();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		else
		{
			selected_shape = -1;
			selected_joint = -1;
		}

		ImGui::EndChild();

		ImGui::EndTable();
	}

	ImGui::End();
}

void NodesWindow::shutdown()
{
	event_connections.disconnectAll();
}
