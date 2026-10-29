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
	PROP_PARAM(Float, arm_length, 6.0f,
		"Arm Length", "Distance from the pivot to the camera", "Spring Arm")

	PROP_GROUP("Rotation")
	PROP_PARAM(Float, mouse_sensitivity, 0.1f,
		"Mouse Sensitivity", "Degrees per raw mouse count", "Rotation")
	PROP_PARAM(Float, min_pitch, -70.0f,
		"Min Pitch", "Minimum arm pitch in degrees", "Rotation")
	PROP_PARAM(Float, max_pitch, 70.0f,
		"Max Pitch", "Maximum arm pitch in degrees", "Rotation")
	PROP_PARAM(Toggle, invert_pitch, false,
		"Invert Pitch", "Invert vertical mouse input", "Rotation")

	PROP_GROUP("Vertical Camera Bias")
	PROP_PARAM(Toggle, enable_min_angle_bias, true,
		"Enable Min Angle Bias", "Blend the camera view upward near the minimum pitch", "Vertical Camera Bias")
	PROP_PARAM(Float, min_angle_bias_offset, 15.0f,
		"Min Angle Bias Offset", "Degrees above Min Pitch over which the view blends toward world up. Zero disables the bias. Independent of arm length and collision.", "Vertical Camera Bias", "enable_min_angle_bias=1")

	PROP_GROUP("Camera Lag")
	PROP_PARAM(Toggle, enable_position_lag, true,
		"Enable Position Lag", "Smooth the spring arm pivot as it follows the target", "Camera Lag")
	PROP_PARAM(Float, position_lag_time_95, 0.625389993f,
		"Horizontal Follow Time (s)", "Seconds to remove 95% of a fixed horizontal pivot error. Larger values increase trailing distance. Zero follows immediately.", "Camera Lag", "enable_position_lag=1")
	PROP_PARAM(Float, position_lag_vertical_time_95, 0.625389993f,
		"Vertical Follow Time (s)", "Seconds to remove 95% of a fixed vertical pivot error. Zero follows height immediately. Independent of horizontal follow.", "Camera Lag", "enable_position_lag=1")
	PROP_PARAM(Float, position_lag_max_distance, 3.0f,
		"Max Pivot Lag (m)", "Hard total-distance limit on pivot lag. Zero disables positional lag. This safety cap can override response times.", "Camera Lag", "enable_position_lag=1")
	PROP_PARAM(Toggle, enable_rotation_lag, false,
		"Enable Rotation Lag", "Enable spring-like lag for camera yaw and pitch", "Camera Lag")
	PROP_PARAM(Float, rotation_lag_time_95, 0.2965f,
		"Rotation Response Time (s)", "Seconds to remove 95% of a fixed angular error from rest. Larger values soften manual rotation. Zero follows immediately. Automatic axes use their own response.", "Camera Lag", "enable_rotation_lag=1")

	PROP_GROUP("Auto Rotation")
	PROP_PARAM(Toggle, enable_auto_rotation, true,
		"Enable Auto Rotation", "Align camera yaw with the target movement direction", "Auto Rotation")
	PROP_PARAM(Float, auto_rotation_time_95, 3.0f,
		"Response Time (s)", "Seconds to remove 95% of a fixed yaw error from rest, without the advanced speed limit. Smooth acceleration is built in. Zero responds immediately subject to the speed limit.", "Auto Rotation", "enable_auto_rotation=1")
	PROP_PARAM(Float, auto_rotation_resume_delay, 1.0f,
		"Resume Delay", "Seconds after the last manual camera input before auto yaw is allowed. Countdown continues while standing. Zero allows immediate activation.", "Auto Rotation", "enable_auto_rotation=1")

	PROP_PARAM(Float, auto_rotation_max_speed, 180.0f,
		"Max Angular Speed (deg/s)", "Maximum automatic yaw speed in degrees per second", "Auto Rotation", "enable_auto_rotation=1")
	PROP_PARAM(Float, auto_rotation_min_move_speed, 1.5f,
		"Min Movement Speed (m/s)", "Minimum horizontal target speed in meters per second", "Auto Rotation", "enable_auto_rotation=1")

	PROP_GROUP("Camera Recovery")
	PROP_PARAM(Toggle, enable_camera_recovery, true,
		"Enable Camera Recovery", "Recover pitch toward the configured comfort range while moving", "Camera Recovery")
	PROP_PARAM(Float, camera_recovery_time_95, 5.164f,
		"Response Time (s)", "Seconds to remove 95% of a fixed pitch error from rest, without the advanced speed limit. Smooth acceleration is built in. Zero responds immediately subject to the speed limit.", "Camera Recovery", "enable_camera_recovery=1")
	PROP_PARAM(Float, camera_recovery_activation_delay, 1.0f,
		"Activation Delay", "Seconds of movement required after manual pitch input before recovery starts. Countdown pauses while standing. Zero allows immediate activation.", "Camera Recovery", "enable_camera_recovery=1")
	PROP_PARAM(Float, camera_recovery_target_pitch_min, 0.0f,
		"Target Pitch Min", "Lower boundary of the comfortable pitch range", "Camera Recovery", "enable_camera_recovery=1")
	PROP_PARAM(Float, camera_recovery_target_pitch_max, 15.0f,
		"Target Pitch Max", "Upper boundary of the comfortable pitch range", "Camera Recovery", "enable_camera_recovery=1")

	PROP_PARAM(Float, camera_recovery_max_speed, 180.0f,
		"Max Angular Speed (deg/s)", "Maximum automatic pitch speed in degrees per second", "Camera Recovery", "enable_camera_recovery=1")
	PROP_PARAM(Float, camera_recovery_min_move_speed, 1.5f,
		"Min Movement Speed (m/s)", "Minimum horizontal target speed in meters per second", "Camera Recovery", "enable_camera_recovery=1")

	PROP_GROUP("Collision")
	PROP_PARAM(Toggle, enable_camera_collision, true,
		"Enable Camera Collision", "Prevent the camera from passing through scene geometry", "Collision")
	PROP_PARAM(Float, collision_sphere_radius, 0.1f,
		"Collision Sphere Radius", "Camera collision clearance radius", "Collision", "enable_camera_collision=1")
	PROP_PARAM(Mask, camera_collision_mask, 1,
		"Camera Collision Mask", "Surface collision and intersection mask used by the camera", "Collision", "enable_camera_collision=1")

	PROP_GROUP("Smooth Zoom After Collision")
	PROP_PARAM(Toggle, enable_smooth_collision_return, true,
		"Enable Smooth Return", "Smoothly restore the spring arm distance after collision", "Smooth Zoom After Collision", "enable_camera_collision=1")
	PROP_PARAM(Float, collision_return_time_95, 0.6f,
		"Return Time (s)", "Seconds to recover 95% of a fixed available arm distance from rest, after Return Delay. Smooth start and finish. Zero returns immediately; inward collision correction is always immediate.", "Smooth Zoom After Collision", "enable_camera_collision=1;enable_smooth_collision_return=1")
	PROP_PARAM(Float, collision_return_delay, 0.1f,
		"Return Delay", "Seconds to wait after an inward collision correction before extending toward the currently safe distance. Zero removes the wait.", "Smooth Zoom After Collision", "enable_camera_collision=1;enable_smooth_collision_return=1")

private:
	void init();
	void update();
	void shutdown();

	void initialize_angles_from_camera();
	void apply_camera_transform();
	void reset_automation();
	void update_target_motion(float ifps);
	void reset_lag_state(const Unigine::Math::Vec3 &target_position);
	Unigine::Math::Vec3 update_position_lag(const Unigine::Math::Vec3 &target_position,
		float ifps);
	void update_auto_rotation(bool manual_camera_input, float ifps);
	void update_camera_recovery(bool manual_pitch_input, float ifps);
	Unigine::Math::vec2 update_rotation_lag(const Unigine::Math::vec2 &target_angles,
		float ifps);
	Unigine::Math::Vec3 resolve_collision(const Unigine::Math::Vec3 &target_position,
		const Unigine::Math::Vec3 &desired_position);
	Unigine::Math::Vec3 update_collision_smooth_return(
		const Unigine::Math::Vec3 &target_position,
		const Unigine::Math::Vec3 &desired_position,
		const Unigine::Math::Vec3 &collision_position,
		float ifps);
	void rebuild_collision_exclude();
	bool is_ignored_collision_object(const Unigine::ObjectPtr &object) const;
	Unigine::Math::Vec3 get_target_position() const;

	Unigine::PlayerDummyPtr camera;
	Unigine::ShapeSpherePtr collision_shape;
	Unigine::WorldIntersectionNormalPtr collision_hit;
	Unigine::Vector<Unigine::ShapeContactPtr> collision_contacts;
	Unigine::Vector<Unigine::NodePtr> collision_exclude;
	Unigine::NodePtr collision_ignore_root;
	Unigine::Math::Vec3 lagged_target_position = Unigine::Math::Vec3_zero;
	Unigine::Math::Vec3 previous_target_position = Unigine::Math::Vec3_zero;
	Unigine::Math::Vec3 target_motion_delta = Unigine::Math::Vec3_zero;
	float target_motion_speed = 0.0f;
	bool target_motion_initialized = false;
	Unigine::Math::vec2 lagged_angles = Unigine::Math::vec2_zero;
	Unigine::Math::vec2 rotation_lag_velocity = Unigine::Math::vec2_zero;
	float auto_rotation_velocity = 0.0f;
	float camera_recovery_velocity = 0.0f;
	float auto_rotation_time_since_input = 0.0f;
	float camera_recovery_delay_remaining = 0.0f;
	float current_collision_distance = 0.0f;
	float collision_return_velocity = 0.0f;
	float collision_return_delay_remaining = 0.0f;
	float yaw = 0.0f;
	float pitch = 0.0f;
	bool angles_initialized = false;
	bool position_lag_initialized = false;
	bool rotation_lag_initialized = false;
	bool auto_rotation_velocity_initialized = false;
	bool auto_rotation_resume_delay_active = false;
	bool auto_rotation_active = false;
	bool camera_recovery_velocity_initialized = false;
	bool camera_recovery_delay_active = false;
	bool camera_recovery_active = false;
	bool collision_return_initialized = false;

	static constexpr int COLLISION_SWEEP_REFINEMENT_ITERATIONS = 6;
	static constexpr int COLLISION_SWEEP_MAX_STEPS = 64;
};
