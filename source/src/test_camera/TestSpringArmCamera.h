#pragma once

#include <UnigineComponentSystem.h>
#include <UniginePhysics.h>
#include <UniginePlayers.h>
#include <UnigineWorld.h>

class TestSpringArmCamera final : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(TestSpringArmCamera, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_GROUP("Spring Arm")
	PROP_PARAM(Node, target)
	PROP_PARAM(Vec3, target_offset, Unigine::Math::vec3_zero,
		"Target Offset", "Target-local pivot offset", "Spring Arm")
	PROP_PARAM(Float, arm_length, 4.0f,
		"Arm Length", "Distance from the pivot to the camera", "Spring Arm")

	PROP_GROUP("Rotation")
	PROP_PARAM(Float, mouse_sensitivity, 0.1f,
		"Mouse Sensitivity", "Degrees per raw mouse count", "Rotation")
	PROP_PARAM(Float, min_pitch, -60.0f,
		"Min Pitch", "Minimum arm pitch in degrees", "Rotation")
	PROP_PARAM(Float, max_pitch, 75.0f,
		"Max Pitch", "Maximum arm pitch in degrees", "Rotation")
	PROP_PARAM(Toggle, invert_pitch, false,
		"Invert Pitch", "Invert vertical mouse input", "Rotation")

	PROP_GROUP("Collision")
	PROP_PARAM(Toggle, enable_camera_collision, true,
		"Enable Camera Collision", "Prevent the camera from passing through scene geometry", "Collision")
	PROP_PARAM(Float, collision_sphere_radius, 0.3f,
		"Collision Sphere Radius", "Camera collision clearance radius", "Collision")
	PROP_PARAM(Mask, camera_collision_mask, 1,
		"Camera Collision Mask", "Surface collision and intersection mask used by the camera", "Collision")

private:
	void init();
	void update();
	void shutdown();

	void initialize_angles_from_camera();
	void apply_camera_transform();
	Unigine::Math::Vec3 resolve_collision(const Unigine::Math::Vec3 &target_position,
		const Unigine::Math::Vec3 &desired_position);
	void rebuild_collision_exclude();
	bool is_ignored_collision_object(const Unigine::ObjectPtr &object) const;
	Unigine::Math::Vec3 get_target_position() const;

	Unigine::PlayerDummyPtr camera;
	Unigine::ShapeSpherePtr collision_shape;
	Unigine::WorldIntersectionNormalPtr collision_hit;
	Unigine::Vector<Unigine::ShapeContactPtr> collision_contacts;
	Unigine::Vector<Unigine::NodePtr> collision_exclude;
	Unigine::NodePtr collision_ignore_root;
	float yaw = 0.0f;
	float pitch = 0.0f;
	bool angles_initialized = false;

	static constexpr int COLLISION_RESOLUTION_ITERATIONS = 4;
};
