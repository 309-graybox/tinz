// Copyright (C), UNIGINE. All rights reserved.
#include "EditorPlayer.h"

#include "RuntimeEditor.h"

#include <UnigineConsole.h>
#include <UnigineEngine.h>
#include <UnigineWindowManager.h>
#include <UnigineWorld.h>

using namespace Unigine;
using namespace Math;

#define BOUND_RADIUS_TO_DISTANCE 3.0f

EditorPlayer::EditorPlayer()
{
	player = PlayerDummy::create();
	player->setName("debug_player");
	player->setLifetime(Node::LIFETIME_ENGINE);
	camera = player->getCamera();

	decompose_player_transform();
}

EditorPlayer::~EditorPlayer()
{
	player.deleteLater();
}

void EditorPlayer::setPhiAngle(float angle)
{
	angle = angle - phi_angle;
	direction = quat(camera->getUp(), angle) * direction;
	phi_angle += angle;

	compose_player_transform();
}

void EditorPlayer::setThetaAngle(float angle)
{
	angle = clamp(angle, min_theta_angle, max_theta_angle) - theta_angle;
	direction = quat(cross(camera->getUp(), direction), angle) * direction;
	theta_angle += angle;

	compose_player_transform();
}

void EditorPlayer::setViewDirection(const vec3 &view)
{
	direction = normalize(view);

	// ortho basis
	vec3 tangent, binormal;
	orthoBasis(camera->getUp(), tangent, binormal);

	// decompose view direction
	phi_angle = Math::atan2(dot(direction, tangent), dot(direction, binormal)) * Consts::RAD2DEG;
	theta_angle =
		Math::acos(clamp(dot(direction, camera->getUp()), -1.0f, 1.0f)) * Consts::RAD2DEG - 90.0f;
	theta_angle = clamp(theta_angle, min_theta_angle, max_theta_angle);

	compose_player_transform();
}

void EditorPlayer::focus(const NodePtr &node, bool attach)
{
	if (!node)
		return;

	Vec3 center = get_node_bound_center(node);
	Scalar radius = get_node_bound_radius(node);

	// compensate for non-standard radius
	Scalar znear = Scalar(player->getZNear() * 5.0f);
	if (radius <= znear || radius >= 1.0e6)
		radius = znear;

	focus(center, float(radius * BOUND_RADIUS_TO_DISTANCE));

	if (attach)
	{
		focus_node = node;
		focus_node_pos = node->getWorldPosition();
	}
}

void EditorPlayer::focus(const Vector<NodePtr> &nodes)
{
	if (nodes.empty())
		return;

	Vec3 center;
	// if (depr::g.manipulators->pivot() == manipulators::PivotType::CENTER)
	//	center = get_nodes_bound_center(nodes);
	// else
	center = get_node_bound_center(nodes.last());

	Scalar radius;
	// if (depr::g.manipulators->pivot() == manipulators::PivotType::CENTER)
	//	radius = get_nodes_bound_radius(nodes);
	// else
	radius = get_node_bound_radius(nodes.last());

	// compensate for non-standard radius
	Scalar znear = Scalar(player->getZNear() * 5.0f);
	if (radius <= znear || radius >= 1.0e6)
		radius = znear;

	focus(center, float(radius * BOUND_RADIUS_TO_DISTANCE));
}

void EditorPlayer::focus(const Vec3 &pos)
{
	focus(pos, distance);
}

void EditorPlayer::focus(const Vec3 &target_pos, float target_distance)
{
	focus_pos = target_pos - Vec3(direction) * target_distance;
	focus_dist = target_distance;
	set_mode(FOCUSING);
}

void EditorPlayer::setTransform(const Mat4 &transform)
{
	player->setTransform(transform);
	decompose_player_transform();
}

void EditorPlayer::setWorldTransform(const Mat4 &transform)
{
	player->setWorldTransform(transform);
	decompose_player_transform();
}

void EditorPlayer::setPosition(const Unigine::Math::Vec3 &position)
{
	player->setPosition(position);
	decompose_player_transform();
}

void EditorPlayer::setWorldPosition(const Unigine::Math::Vec3 &in_position)
{
	player->setWorldPosition(in_position);
	position = in_position;
}

void EditorPlayer::setRotation(const Unigine::Math::quat &rotation)
{
	player->setRotation(rotation);
	decompose_player_transform();
}

void EditorPlayer::setWorldRotation(const Unigine::Math::quat &rotation)
{
	player->setWorldRotation(rotation);
	decompose_player_transform();
}

void EditorPlayer::update()
{
	update_mouse();

	// following node
	if (focus_node)
	{
		Vec3 node_pos = focus_node->getWorldPosition();
		Vec3 diff = node_pos - focus_node_pos;
		focus_pos += diff;
		position += diff;
		focus_node_pos = node_pos;
		compose_player_transform();

		// detach: MMB or RMB (without alt key - dolly mode)
		if (RuntimeEditor::isMouseAvailable()
			&& (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_MIDDLE)
				|| (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT)
					&& !Input::isKeyPressed(Input::KEY_ANY_ALT)
					&& RuntimeEditor::isKeyboardAvailable())))
		{
			focus_node = NodePtr();
		}
	}

	if (mode == FOCUSING)
	{
		update_focus();
		return;
	}

	if (!controlled || !RuntimeEditor::isMouseAvailable())
	{
		set_mode(IDLE);
		return;
	}

	if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_MIDDLE))
	{
		set_mode(PAN);
		update_pan();
	}
	else if (RuntimeEditor::isKeyboardAvailable() && Input::isKeyPressed(Input::KEY_ANY_ALT)
			 && Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT))
	{
		set_mode(ORBIT);
		update_orbit();
	}
	else if ((RuntimeEditor::isKeyboardAvailable() && Input::isKeyPressed(Input::KEY_ANY_ALT)
				 && Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT))
			 || Input::getMouseWheel() != 0)
	{
		set_mode(DOLLY);
		update_dolly();
	}
	else if (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT))
	{
		set_mode(FREE);
		update_free();
	}
	else
	{
		set_mode(IDLE);
	}
}

void EditorPlayer::set_mode(MODE next_mode)
{
	mode = next_mode;
}

void EditorPlayer::update_mouse()
{
	auto getMousePosition = []() {
		auto window = Unigine::WindowManager::getMainWindow();
		if (!window)
			return Unigine::Math::ivec2_zero;

		Unigine::Math::ivec2 window_pos = window->getClientPosition();
		return Unigine::Input::getMousePosition() - window_pos;
	};

	ivec2 mouse_pos = getMousePosition();
	mouse_delta = vec2(mouse_pos - mouse_prev_pos) * mouse_sensitivity;
	auto window = WindowManager::getMainWindow();
	if (!window)
		return;

	Unigine::Math::ivec2 window_size = window->getClientSize();
	window_size = Unigine::Math::ivec2(
		Unigine::Math::vec2(window_size) * WindowManager::getMainWindow()->getDpiScale());
	if (window_size.length2() <= 1)
		return;

	// loop mode
	bool out_of_border = false;
	if (mode != IDLE)
	{
		auto negative_mod = [](int x, int y) { return y - Math::abs(x % y); };

		if (mouse_pos.x >= window_size.x - 1)
		{
			mouse_pos.x = (mouse_pos.x + 1) % window_size.x + 1;
			out_of_border = true;
		}
		else if (mouse_pos.x <= 0)
		{
			mouse_pos.x = negative_mod(mouse_pos.x - 1, window_size.x) - 1;
			out_of_border = true;
		}

		if (mouse_pos.y >= window_size.y - 1)
		{
			mouse_pos.y = (mouse_pos.y + 1) % window_size.y + 1;
			out_of_border = true;
		}
		else if (mouse_pos.y <= 0)
		{
			mouse_pos.y = negative_mod(mouse_pos.y - 1, window_size.y) - 1;
			out_of_border = true;
		}

		if (out_of_border)
			Input::setMousePosition(window->getClientPosition() + mouse_pos);
	}

	// directions for panning
	prev_mouse_direction = mouse_direction;
	mouse_direction = player->getDirectionFromMainWindow(mouse_pos.x, mouse_pos.y);
	if (out_of_border)
		prev_mouse_direction = mouse_direction;

	mouse_prev_pos = mouse_pos;
}

void EditorPlayer::update_free()
{
	float ifps = Engine::get()->getIFps();

	// ortho basis
	vec3 up = camera->getUp();
	vec3 tangent, binormal;
	orthoBasis(up, tangent, binormal);

	// direction
	phi_angle += mouse_delta.x;
	theta_angle += mouse_delta.y;
	if (Input::isKeyPressed(Input::KEY_UP))
		theta_angle -= turning * ifps;
	if (Input::isKeyPressed(Input::KEY_DOWN))
		theta_angle += turning * ifps;
	if (Input::isKeyPressed(Input::KEY_LEFT))
		phi_angle -= turning * ifps;
	if (Input::isKeyPressed(Input::KEY_RIGHT))
		phi_angle += turning * ifps;
	theta_angle = clamp(theta_angle, min_theta_angle, max_theta_angle);

	// new basis
	vec3 x = (quat(up, -phi_angle) * quat(tangent, -theta_angle)) * binormal;
	vec3 y = normalize(cross(up, x));
	vec3 z = normalize(cross(x, y));

	// direction
	direction = x;

	// impulse
	vec3 impulse;
	if (Input::isKeyPressed(Input::KEY_W))
		impulse += x;
	if (Input::isKeyPressed(Input::KEY_S))
		impulse -= x;
	if (Input::isKeyPressed(Input::KEY_A))
		impulse += y;
	if (Input::isKeyPressed(Input::KEY_D))
		impulse -= y;
	if (Input::isKeyPressed(Input::KEY_Q))
		impulse -= z;
	if (Input::isKeyPressed(Input::KEY_E))
		impulse += z;
	impulse.normalize();

	// velocity
	impulse *= velocity;
	if (Input::isKeyPressed(Input::KEY_ANY_SHIFT))
		impulse *= 2;

	// movement
	position += Vec3(impulse * ifps);

	compose_player_transform();
}

void EditorPlayer::update_orbit()
{
	float ifps = Engine::get()->getIFps();

	// ortho basis
	vec3 up = camera->getUp();
	vec3 tangent, binormal;
	orthoBasis(up, tangent, binormal);

	// direction
	phi_angle += mouse_delta.x;
	theta_angle += mouse_delta.y;
	if (Input::isKeyPressed(Input::KEY_UP))
		theta_angle -= turning * ifps;
	if (Input::isKeyPressed(Input::KEY_DOWN))
		theta_angle += turning * ifps;
	if (Input::isKeyPressed(Input::KEY_LEFT))
		phi_angle -= turning * ifps;
	if (Input::isKeyPressed(Input::KEY_RIGHT))
		phi_angle += turning * ifps;
	theta_angle = clamp(theta_angle, min_theta_angle, max_theta_angle);

	// new basis
	vec3 x = (quat(up, -phi_angle) * quat(tangent, -theta_angle)) * binormal;

	// orbit
	Vec3 target_pos = position + Vec3(direction) * distance;
	position = target_pos - Vec3(x) * distance;
	direction = x;

	compose_player_transform();
}

void EditorPlayer::update_dolly()
{
	float offset = (Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT) ? -mouse_delta.y : 0.0f)
				   - Input::getMouseWheel() * 10.0f;

	// closer to center means smaller steps
	offset *= Math::clamp((Math::max(distance, 1.0f) + Consts::EPS) / 500.0f, 0.0f, 1.0f);

	offset *= velocity;
	if (Input::isKeyPressed(Input::KEY_ANY_SHIFT))
		offset *= 2;

	// clamp to zero (min distance)
	if (distance + offset < 0)
		offset = -distance;
	distance += offset;

	// dolly
	position += -Vec3(direction) * offset;

	compose_player_transform();
}

void EditorPlayer::update_pan()
{
	auto get_plane_intersecton = [](const Vec3 &ray_origin, const vec3 &ray_vector,
									 const Vec3 &plane_point, const vec3 &plane_normal) -> Vec3 {
		return ray_origin
			   - Vec3(ray_vector) * dot(ray_origin - plane_point, Vec3(plane_normal))
					 / dot(ray_vector, plane_normal);
	};

	const Vec3 camera_position = player->getWorldPosition();
	const vec3 camera_direction = player->getWorldDirection();

	const Vec3 plane_point = camera_position + Vec3(camera_direction) * max(distance, 1.0f);
	const vec3 plane_normal = -camera_direction;

	const Vec3 old_intersection =
		get_plane_intersecton(camera_position, prev_mouse_direction, plane_point, plane_normal);
	const Vec3 new_intersection =
		get_plane_intersecton(camera_position, mouse_direction, plane_point, plane_normal);

	position -= new_intersection - old_intersection;

	compose_player_transform();
}

void EditorPlayer::update_focus()
{
	Vec3 shift = (focus_pos - position) * Engine::get()->getIFps() * 10;
	Vec3 new_position = position + shift;

	Scalar shift_length = length(shift);
	Scalar delta = length(focus_pos - position);
	Scalar relative_delta = delta / focus_dist;

	if (relative_delta <= 1e-3f || shift_length >= delta)
	{
		// jump to target position
		position = focus_pos;
		distance = focus_dist;
		set_mode(IDLE);
	}
	else
		position = new_position;

	compose_player_transform();
}

void EditorPlayer::decompose_player_transform()
{
	// get ortho basis
	vec3 up = camera->getUp();
	vec3 tangent, binormal;
	orthoBasis(up, tangent, binormal);

	// decompose transformation
	Mat4 transform = player->getWorldTransform();
	position = transform.getColumn3(3);
	direction = normalize(vec3(-transform.getColumn3(2)));

	phi_angle = Math::atan2(dot(direction, tangent), dot(direction, binormal)) * Consts::RAD2DEG;
	theta_angle = Math::acos(clamp(dot(direction, up), -1.0f, 1.0f)) * Consts::RAD2DEG - 90.0f;
	theta_angle = clamp(theta_angle, min_theta_angle, max_theta_angle);
}

void EditorPlayer::compose_player_transform()
{
	player->setWorldTransform(setTo(position, position + Vec3(direction), camera->getUp()));
}

Scalar EditorPlayer::get_node_bound_radius(const NodePtr &node)
{
	if (node->getType() == Node::OBJECT_PARTICLES)
	{
		auto particles = static_ptr_cast<ObjectParticles>(node);
		auto radius = particles->getRadiusOverTimeModifier()->getMaxValue();
		return static_cast<Scalar>(radius);
	}

	if (node->getType() == Node::OBJECT_WATER_GLOBAL)
	{
		auto water = static_ptr_cast<ObjectWaterGlobal>(node);
		return static_cast<Scalar>(water->getWaterlineSize());
	}

	if (node->getType() == Node::LANDSCAPE_LAYER_MAP)
	{
		auto map = static_ptr_cast<LandscapeLayerMap>(node);
		auto rect = Unigine::Math::max(map->getExtremumHeight(), map->getSize() * 0.5f);
		return static_cast<Scalar>(rect.max());
	}

	Scalar radius = node->getWorldBoundSphere().radius;
	if (radius < Scalar(Consts::EPS) || radius >= 1.0e6)
		radius = Scalar(1.0f);

	return radius;
}

Vec3 EditorPlayer::get_node_bound_center(const NodePtr &node)
{
	int node_type = node->getType();
	if (node_type != Node::NODE_DUMMY && node_type != Node::LIGHT_WORLD
		&& node_type != Node::OBJECT_WATER_GLOBAL && node_type != Node::OBJECT_PARTICLES)
	{
		auto sphere = node->getWorldBoundSphere();
		if (sphere.isValid())
			return sphere.center;

		auto box = node->getWorldBoundBox();
		if (box.isValid())
			return Vec3(box.minimum + box.maximum) * 0.5f;
	}
	return node->getWorldPosition();
}

Scalar EditorPlayer::get_nodes_bound_radius(const Vector<NodePtr> &nodes)
{
	if (nodes.empty())
		return 0.0f;

	vec3 center = vec3(get_node_bound_center(nodes.first()));
	float radius = float(get_node_bound_radius(nodes.first()));
	BoundSphere bs = BoundSphere(center, radius);
	for (int i = 1, count = nodes.size(); i < count; i++)
	{
		center = vec3(get_node_bound_center(nodes[i]));
		radius = float(get_node_bound_radius(nodes[i]));
		bs.expand(BoundSphere(center, radius));
	}

	return bs.radius;
}

Vec3 EditorPlayer::get_nodes_bound_center(const Vector<NodePtr> &nodes)
{
	if (nodes.empty())
		return Unigine::Math::Vec3_zero;

	const Vec3 center_first = get_node_bound_center(nodes.first());
	Vec3 min_center = center_first;
	Vec3 max_center = center_first;
	for (int i = 1, count = nodes.size(); i < count; ++i)
	{
		const Vec3 center = get_node_bound_center(nodes[i]);
		min_center = min(min_center, center);
		max_center = max(max_center, center);
	}

	return (min_center + max_center) * Scalar(0.5);
}
