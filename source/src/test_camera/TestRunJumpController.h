#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineNodes.h>
#include <UnigineObjects.h>
#include <UniginePhysics.h>

class TestRunJumpController final : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(TestRunJumpController, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_GROUP("Run")
	PROP_PARAM(Float, run_speed, 5.0f,
		"Run Speed", "Maximum running speed", "Run")
	PROP_PARAM(Float, acceleration, 15.0f,
		"Acceleration", "Rate at which velocity approaches the requested run velocity", "Run")
	PROP_PARAM(Float, damping, 23.0f,
		"Damping", "Rate at which velocity approaches zero without movement input", "Run")
	PROP_PARAM(Float, turn_speed, 600.0f,
		"Turn Speed", "Character turn speed in degrees per second", "Run")

	PROP_GROUP("Jump")
	PROP_PARAM(Float, jump_speed, 5.5f,
		"Jump Speed", "Initial upward speed", "Jump")
	PROP_PARAM(Float, adaptive_jump_damping, 0.6f,
		"Adaptive Jump Damping", "Fraction of upward speed removed when jump is released", "Jump")
	PROP_PARAM(Float, adaptive_jump_threshold, 0.05f,
		"Adaptive Jump Threshold", "Minimum upward speed as a fraction of Jump Speed", "Jump")
	PROP_PARAM(Mask, jump_surface_intersection_mask, ~1,
		"Jump Surface Intersection Mask", "Surfaces that can be used as jump support", "Jump")

	PROP_GROUP("Collision Foundation")
	PROP_PARAM(Mask, movement_collision_mask, 1,
		"Movement Collision Mask", "Shape collision mask used by the controller", "Collision Foundation")
	PROP_PARAM(Float, max_ground_angle, 43.0f,
		"Max Ground Angle", "Maximum walkable ground angle in degrees", "Collision Foundation")
	PROP_PARAM(Int, collision_iterations, 4,
		"Collision Iterations", "Number of penetration resolution iterations", "Collision Foundation")

	bool isGrounded() const { return grounded; }
	Unigine::Math::vec3 getHorizontalVelocity() const { return horizontal_velocity; }
	float getVerticalSpeed() const { return vertical_speed; }

private:
	void init();
	void update();

	Unigine::Math::vec3 read_move_direction() const;
	void update_horizontal_velocity(const Unigine::Math::vec3 &move_direction, float ifps);
	void rotate_towards(const Unigine::Math::vec3 &direction, float ifps);
	void resolve_collisions(float ifps);
	void collect_animation_poses(const Unigine::NodePtr &root);
	void update_animation_graphs(bool landed);
	static void move_towards(Unigine::Math::vec3 &current,
		const Unigine::Math::vec3 &target, float max_delta);

	Unigine::ObjectPtr object;
	Unigine::BodyDummyPtr body;
	Unigine::ShapeCapsulePtr shape;
	Unigine::Vector<Unigine::ShapeContactPtr> contacts;
	Unigine::Vector<Unigine::NodeSkeletonPosePtr> animation_poses;

	Unigine::Math::Mat4 world_transform = Unigine::Math::Mat4_identity;
	Unigine::Math::vec3 horizontal_velocity = Unigine::Math::vec3_zero;
	Unigine::Math::vec3 up = Unigine::Math::vec3_up;
	float gravity_amount = 0.0f;
	float vertical_speed = 0.0f;
	float ground_normal_cos = 0.0f;

	bool controller_ready = false;
	bool grounded = false;
	bool can_jump = false;
	bool adaptive_jump_pending = false;
	bool ground_state_initialized = false;
};
