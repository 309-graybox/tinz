// Copyright (C), UNIGINE. All rights reserved.
#pragma once
#include <UnigineControls.h>
#include <UnigineGame.h>
#include <UnigineNode.h>
#include <UniginePlayers.h>

class EditorPlayer
{
public:
	enum MODE {
		IDLE,		 // Default
		FREE,		 // RMB
		ORBIT,		 // Alt + LMB
		DOLLY,		 // Alt + RMB or Mouse Wheel
		PAN,		 // MMB
		FOCUSING,	 // focus()
	};

	EditorPlayer();
	~EditorPlayer();

	const Unigine::PlayerDummyPtr &getPlayer() const { return player; }

	// control
	void setControlled(bool enabled) { controlled = enabled; }
	bool isControlled() const { return controlled; }

	// velocity
	void setVelocity(float value) { velocity = Unigine::Math::max(value, 0.0f); }
	float getVelocity() const { return velocity; }

	// minimum theta angle
	void setMinThetaAngle(float angle)
	{
		min_theta_angle = Unigine::Math::clamp(angle, -89.9f, 89.9f);
	}
	float getMinThetaAngle() const { return min_theta_angle; }

	// maximum theta angle
	void setMaxThetaAngle(float angle)
	{
		max_theta_angle = Unigine::Math::clamp(angle, -89.9f, 89.9f);
	}
	float getMaxThetaAngle() const { return max_theta_angle; }

	// turning
	void setTurning(float value) { turning = Unigine::Math::max(value, 0.0f); }
	float getTurning() const { return turning; }

	// phi (horizontal) angle
	void setPhiAngle(float angle);
	float getPhiAngle() const { return phi_angle; }

	// theta (vertical) angle
	void setThetaAngle(float angle);
	float getThetaAngle() const { return theta_angle; }

	// view direction
	void setViewDirection(const Unigine::Math::vec3 &view);
	const Unigine::Math::vec3 &getViewDirection() const { return direction; }

	// orbit settings
	void setOrbitDistance(float value) { distance = value; }
	float getOrbitDistance() const { return distance; }

	// focus
	void focus(const Unigine::NodePtr &node, bool attach = false);
	void focus(const Unigine::Vector<Unigine::NodePtr> &nodes);
	void focus(const Unigine::Math::Vec3 &pos);
	void focus(const Unigine::Math::Vec3 &pos, float distance);

	// mode
	MODE getMode() const { return mode; }

	// transform
	void setTransform(const Unigine::Math::Mat4 &transform);
	void setWorldTransform(const Unigine::Math::Mat4 &transform);
	void setPosition(const Unigine::Math::Vec3 &position);
	void setWorldPosition(const Unigine::Math::Vec3 &position);
	void setRotation(const Unigine::Math::quat &rotation);
	void setWorldRotation(const Unigine::Math::quat &rotation);

public:
	// world main loop
	void update();

private:
	void set_mode(MODE mode);
	void update_mouse();
	void update_free();
	void update_orbit();
	void update_dolly();
	void update_pan();
	void update_focus();
	void decompose_player_transform();
	void compose_player_transform();
	Unigine::Math::Scalar get_node_bound_radius(const Unigine::NodePtr &node);
	Unigine::Math::Vec3 get_node_bound_center(const Unigine::NodePtr &node);
	Unigine::Math::Scalar get_nodes_bound_radius(const Unigine::Vector<Unigine::NodePtr> &nodes);
	Unigine::Math::Vec3 get_nodes_bound_center(const Unigine::Vector<Unigine::NodePtr> &nodes);

	MODE mode = MODE::IDLE;

	Unigine::PlayerDummyPtr player;
	Unigine::CameraPtr camera;

	// free
	bool controlled = true;
	float velocity = 5.0f;
	float turning = 90.0f;
	float min_theta_angle = -89.9f;
	float max_theta_angle = 89.9f;
	float mouse_sensitivity = 0.2f;

	Unigine::Math::Vec3 position;
	Unigine::Math::vec3 direction;
	float phi_angle;
	float theta_angle;

	// orbit
	float distance = 5.0f;

	// pan
	Unigine::Math::vec3 mouse_direction;
	Unigine::Math::vec3 prev_mouse_direction;

	// focus
	Unigine::Math::Vec3 focus_pos;
	float focus_dist;

	// focus: attach
	Unigine::NodePtr focus_node;
	Unigine::Math::Vec3 focus_node_pos;

	// mouse
	Unigine::Math::ivec2 mouse_prev_pos;
	Unigine::Math::vec2 mouse_delta;
};
