#include "TestSpringArmCamera.h"

#include <cmath>

#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineMathLib.h>
#include <UnigineWorld.h>

using namespace Unigine;
using namespace Unigine::Math;

namespace
{
// Response calibration: exp(-3) ~= .05; (1 + 4.744) * exp(-4.744) ~= .05.
float exponential_alpha(float time95, float dt)
{
	return time95 > Consts::EPS ? 1.0f - std::exp(-3.0f * dt / time95) : 1.0f;
}

float wrap_angle(float angle)
{
	while (angle > 180.0f) angle -= 360.0f;
	while (angle < -180.0f) angle += 360.0f;
	return angle;
}

// Exact critical spring for a stationary target during this step.
// Restrictions (speed caps, collision safety) belong to the caller.
template <typename T>
T spring_step(const T &current, const T &target, T &velocity, float time95, float dt)
{
	const float omega = 4.744f / time95;
	const float decay = std::exp(-omega * dt);
	const T change = current - target;
	const T temp = (velocity + omega * change) * dt;
	velocity = (velocity - omega * temp) * decay;
	return target + (change + temp) * decay;
}
}

REGISTER_COMPONENT(TestSpringArmCamera)

void TestSpringArmCamera::init()
{
	camera = checked_ptr_cast<PlayerDummy>(node);
	if (!camera || !target)
		return;

	collision_shape = ShapeSphere::create(max(0.0f, static_cast<float>(collision_sphere_radius)));
	collision_shape->setContinuous(false);
	collision_shape->setCollisionMask(camera_collision_mask);
	collision_hit = WorldIntersectionNormal::create();

	initialize_angles_from_camera();
	apply_camera_transform();
}

void TestSpringArmCamera::update()
{
	if (!camera || !target)
	{
		angles_initialized = false;
		position_lag_initialized = false;
		rotation_lag_initialized = false;
		reset_automation();
		collision_return_initialized = false;
		return;
	}

	if (!angles_initialized)
		initialize_angles_from_camera();

	const float ifps = max(0.0f, Game::getIFps());
	const bool accepts_camera_input = camera->isMainPlayer() && Input::isMouseCursorHide();
	bool manual_camera_input = false;
	bool manual_pitch_input = false;
	if (accepts_camera_input)
	{
		const ivec2 mouse_delta = Input::getMouseDeltaRaw();
		manual_camera_input = mouse_delta.x != 0 || mouse_delta.y != 0;
		manual_pitch_input = mouse_delta.y != 0;
		const float sensitivity = mouse_sensitivity;

		yaw += static_cast<float>(mouse_delta.x) * sensitivity;

		float pitch_delta = static_cast<float>(mouse_delta.y) * sensitivity;
		if (invert_pitch)
			pitch_delta = -pitch_delta;
		pitch += pitch_delta;

		pitch = clamp(pitch, static_cast<float>(min_pitch), static_cast<float>(max_pitch));
		yaw = wrap_angle(yaw);
	}

	if (accepts_camera_input)
	{
		update_target_motion(ifps);
		update_auto_rotation(manual_camera_input, ifps);
		update_camera_recovery(manual_pitch_input, ifps);
	}
	else
	{
		reset_automation();
	}

	apply_camera_transform();
}

void TestSpringArmCamera::shutdown()
{
	if (collision_shape)
		collision_shape.deleteLater();
}

void TestSpringArmCamera::initialize_angles_from_camera()
{
	vec3 direction(get_target_position() - camera->getWorldPosition());
	if (direction.length2() < 1e-8f)
		direction = camera->getWorldDirection();
	direction.normalizeValid();

	vec3 horizontal_direction = direction;
	horizontal_direction.z = 0.0f;

	if (horizontal_direction.length2() < 1e-8f)
	{
		yaw = 0.0f;
		pitch = direction.z < 0.0f ? 89.9f : -89.9f;
	}
	else
	{
		horizontal_direction.normalize();

		pitch = getAngle(direction, horizontal_direction) * -sign(direction.z);
		yaw = getAngle(horizontal_direction, vec3_forward) * sign(direction.x);
	}

	pitch = clamp(pitch, static_cast<float>(min_pitch), static_cast<float>(max_pitch));
	angles_initialized = true;
	reset_lag_state(get_target_position());
}

void TestSpringArmCamera::apply_camera_transform()
{
	const float ifps = max(0.0f, Game::getIFps());
	const vec2 applied_angles = update_rotation_lag(vec2(yaw, pitch), ifps);

	vec3 camera_direction = vec3_forward * rotateZ(applied_angles.x);
	vec3 camera_right = cross(camera_direction, vec3_up);
	camera_right.normalizeValid();
	camera_direction = camera_direction * rotate(camera_right, applied_angles.y);
	camera_direction.normalize();

	const float distance = max(0.0f, static_cast<float>(arm_length));
	const Vec3 target_position = get_target_position();
	const Vec3 lagged_position = update_position_lag(target_position, ifps);
	const Vec3 desired_position = lagged_position - Vec3(camera_direction * distance);
	const Vec3 collision_position = resolve_collision(lagged_position, desired_position);
	const Vec3 camera_position = update_collision_smooth_return(
		lagged_position, desired_position, collision_position, ifps);

	vec3 look_direction(lagged_position - camera_position);
	if (look_direction.length2() < 1e-8f)
		look_direction = camera_direction;
	look_direction.normalizeValid();

	if (enable_min_angle_bias)
	{
		const float bias_range = max(0.0f, static_cast<float>(min_angle_bias_offset));
		float bias_weight = 0.0f;
		if (bias_range > Consts::EPS)
		{
			const float bias_end_pitch = static_cast<float>(min_pitch) + bias_range;
			bias_weight = clamp((bias_end_pitch - applied_angles.y) / bias_range, 0.0f, 1.0f);
		}

		look_direction = look_direction * (1.0f - bias_weight) + vec3_up * bias_weight;
	}
	look_direction.normalizeValid();
	vec3 view_up = cross(camera_right, look_direction);
	view_up.normalizeValid();

	camera->setWorldDirection(look_direction, view_up);
	camera->setWorldPosition(camera_position);
}

void TestSpringArmCamera::reset_lag_state(const Vec3 &target_position)
{
	lagged_target_position = target_position;
	lagged_angles = vec2(yaw, pitch);
	rotation_lag_velocity = vec2_zero;
	position_lag_initialized = true;
	rotation_lag_initialized = true;
	reset_automation();
	collision_return_initialized = false;
}

void TestSpringArmCamera::reset_automation()
{
	target_motion_initialized = false;
	auto_rotation_time_since_input = 0.0f;
	auto_rotation_velocity = 0.0f;
	auto_rotation_velocity_initialized = false;
	auto_rotation_resume_delay_active = false;
	auto_rotation_active = false;
	camera_recovery_velocity_initialized = false;
	camera_recovery_active = false;
	camera_recovery_velocity = 0.0f;
	camera_recovery_delay_remaining = 0.0f;
	camera_recovery_delay_active = false;
}

void TestSpringArmCamera::update_target_motion(float ifps)
{
	if (ifps <= 0.0f)
		return;
	const Vec3 position = target->getWorldPosition();
	target_motion_delta = target_motion_initialized
		? position - previous_target_position : Vec3_zero;
	target_motion_delta.z = 0.0;
	target_motion_speed = static_cast<float>(target_motion_delta.length()) / ifps;
	previous_target_position = position;
	target_motion_initialized = true;
}

Vec3 TestSpringArmCamera::update_position_lag(const Vec3 &target_position, float ifps)
{
	if (!position_lag_initialized || !enable_position_lag)
	{
		lagged_target_position = target_position;
		position_lag_initialized = true;
		return lagged_target_position;
	}
	if (ifps <= 0.0f)
		return lagged_target_position;

	const float horizontal_time = max(0.0f, static_cast<float>(position_lag_time_95));
	const float vertical_time = max(0.0f, static_cast<float>(position_lag_vertical_time_95));
	const float horizontal_alpha = exponential_alpha(horizontal_time, ifps);
	const float vertical_alpha = exponential_alpha(vertical_time, ifps);
	const Vec3 error = target_position - lagged_target_position;
	lagged_target_position += Vec3(error.x * horizontal_alpha,
		error.y * horizontal_alpha, error.z * vertical_alpha);
	const float limit = max(0.0f, static_cast<float>(position_lag_max_distance));
	const Vec3 offset = lagged_target_position - target_position;
	const double distance = static_cast<double>(offset.length());
	if (distance > static_cast<double>(limit) && distance > 0.0)
		lagged_target_position = target_position + offset * static_cast<float>(limit / distance);
	return lagged_target_position;
}

void TestSpringArmCamera::update_auto_rotation(bool manual_camera_input, float ifps)
{
	if (!target || ifps <= 0.0f)
		return;

	if (!auto_rotation_velocity_initialized || !enable_auto_rotation)
	{
		auto_rotation_time_since_input = 0.0f;
		auto_rotation_velocity = 0.0f;
		auto_rotation_velocity_initialized = true;
		auto_rotation_resume_delay_active = false;
		auto_rotation_active = false;
		return;
	}

	const Vec3 &horizontal_delta = target_motion_delta;
	const float horizontal_speed = target_motion_speed;
	const float min_move_speed = max(0.0f, static_cast<float>(auto_rotation_min_move_speed));
	const bool is_moving = horizontal_speed > Consts::EPS && horizontal_speed >= min_move_speed;

	if (manual_camera_input)
	{
		auto_rotation_time_since_input = 0.0f;
		auto_rotation_resume_delay_active = true;
		auto_rotation_velocity = 0.0f;
		auto_rotation_active = false;
		return;
	}

	if (auto_rotation_resume_delay_active)
	{
		auto_rotation_time_since_input += ifps;
		if (auto_rotation_time_since_input
			< max(0.0f, static_cast<float>(auto_rotation_resume_delay)))
		{
			auto_rotation_velocity = 0.0f;
			auto_rotation_active = false;
			return;
		}
		auto_rotation_resume_delay_active = false;
	}

	if (!is_moving)
	{
		auto_rotation_velocity = 0.0f;
		auto_rotation_active = false;
		return;
	}

	if (!auto_rotation_active)
	{
		if (enable_rotation_lag && rotation_lag_initialized)
			yaw = lagged_angles.x;
		auto_rotation_active = true;
		auto_rotation_velocity = 0.0f;
	}

	vec3 movement_direction(horizontal_delta);
	movement_direction.normalizeValid();
	vec3 camera_forward = camera->getViewDirection();
	camera_forward.z = 0.0f;
	camera_forward.normalizeValid();
	const float forward_alignment = dot(movement_direction, camera_forward);
	if (forward_alignment < 0.0f)
	{
		movement_direction -= camera_forward * (2.0f * forward_alignment);
		movement_direction.normalizeValid();
	}
	const float target_yaw = atan2(movement_direction.x, movement_direction.y) * Consts::RAD2DEG;
	float yaw_error = target_yaw - yaw;
	yaw_error = wrap_angle(yaw_error);

	const float response_time = max(0.0f, static_cast<float>(auto_rotation_time_95));
	const float max_speed = max(0.0f, static_cast<float>(auto_rotation_max_speed));
	float step = yaw_error;
	if (response_time > Consts::EPS)
	{
		step = spring_step(0.0f, yaw_error, auto_rotation_velocity, response_time, ifps);
		auto_rotation_velocity = clamp(auto_rotation_velocity, -max_speed, max_speed);
	}
	else
		auto_rotation_velocity = 0.0f;
	yaw += clamp(step, -max_speed * ifps, max_speed * ifps);
	yaw = wrap_angle(yaw);
}

void TestSpringArmCamera::update_camera_recovery(bool manual_pitch_input, float ifps)
{
	if (!target || ifps <= 0.0f)
		return;

	if (!camera_recovery_velocity_initialized || !enable_camera_recovery)
	{
		camera_recovery_delay_remaining = 0.0f;
		camera_recovery_velocity = 0.0f;
		camera_recovery_velocity_initialized = true;
		camera_recovery_delay_active = false;
		camera_recovery_active = false;
		return;
	}

	const float horizontal_speed = target_motion_speed;
	const bool is_moving = horizontal_speed > Consts::EPS && horizontal_speed
		>= max(0.0f, static_cast<float>(camera_recovery_min_move_speed));

	if (manual_pitch_input)
	{
		camera_recovery_delay_remaining = max(0.0f,
			static_cast<float>(camera_recovery_activation_delay));
		camera_recovery_delay_active = true;
		camera_recovery_velocity = 0.0f;
		camera_recovery_active = false;
		return;
	}

	if (camera_recovery_delay_active)
	{
		if (!is_moving)
			return;

		camera_recovery_delay_remaining = max(0.0f,
			camera_recovery_delay_remaining - ifps);
		if (camera_recovery_delay_remaining > 0.0f)
			return;
		camera_recovery_delay_active = false;
	}

	if (!is_moving)
	{
		camera_recovery_velocity = 0.0f;
		camera_recovery_active = false;
		return;
	}

	const float target_min = min(static_cast<float>(camera_recovery_target_pitch_min),
		static_cast<float>(camera_recovery_target_pitch_max));
	const float target_max = max(static_cast<float>(camera_recovery_target_pitch_min),
		static_cast<float>(camera_recovery_target_pitch_max));
	if (!camera_recovery_active && enable_rotation_lag && rotation_lag_initialized
		&& (pitch < target_min || pitch > target_max))
		pitch = lagged_angles.y;
	const float target_pitch = clamp(pitch, target_min, target_max);
	const float pitch_error = target_pitch - pitch;
	if (abs(pitch_error) <= Consts::EPS)
	{
		camera_recovery_velocity = 0.0f;
		camera_recovery_active = false;
		return;
	}

	if (!camera_recovery_active)
	{
		camera_recovery_active = true;
		camera_recovery_velocity = 0.0f;
	}
	const float response_time = max(0.0f, static_cast<float>(camera_recovery_time_95));
	const float max_speed = max(0.0f, static_cast<float>(camera_recovery_max_speed));
	float step = pitch_error;
	if (response_time > Consts::EPS)
	{
		step = spring_step(0.0f, pitch_error, camera_recovery_velocity, response_time, ifps);
		camera_recovery_velocity = clamp(camera_recovery_velocity, -max_speed, max_speed);
	}
	else
		camera_recovery_velocity = 0.0f;
	pitch += clamp(step, -max_speed * ifps, max_speed * ifps);
	pitch = clamp(pitch, static_cast<float>(min_pitch), static_cast<float>(max_pitch));
}

vec2 TestSpringArmCamera::update_rotation_lag(const vec2 &target_angles, float ifps)
{
	if (!rotation_lag_initialized || !enable_rotation_lag)
	{
		lagged_angles = target_angles;
		rotation_lag_velocity = vec2_zero;
		rotation_lag_initialized = true;
		return lagged_angles;
	}

	if (ifps <= 0.0f)
		return lagged_angles;
	const float response_time = max(0.0f, static_cast<float>(rotation_lag_time_95));
	if (response_time <= Consts::EPS)
	{
		lagged_angles = target_angles;
		rotation_lag_velocity = vec2_zero;
		return lagged_angles;
	}

	vec2 unwrapped_target = target_angles;
	float yaw_delta = unwrapped_target.x - lagged_angles.x;
	yaw_delta = wrap_angle(yaw_delta);
	unwrapped_target.x = lagged_angles.x + yaw_delta;

	lagged_angles = spring_step(lagged_angles, unwrapped_target,
		rotation_lag_velocity, response_time, ifps);
	// Automatic axes already have their own spring. Do not filter them twice.
	if (auto_rotation_active)
	{
		lagged_angles.x = target_angles.x;
		rotation_lag_velocity.x = 0.0f;
	}
	if (camera_recovery_active)
	{
		lagged_angles.y = target_angles.y;
		rotation_lag_velocity.y = 0.0f;
	}

	lagged_angles.x = wrap_angle(lagged_angles.x);
	lagged_angles.y = clamp(lagged_angles.y,
		static_cast<float>(min_pitch), static_cast<float>(max_pitch));
	return lagged_angles;
}

Vec3 TestSpringArmCamera::resolve_collision(const Vec3 &target_position,
	const Vec3 &desired_position)
{
	if (!enable_camera_collision || !collision_shape || !collision_hit)
		return desired_position;

	const float radius = max(0.0f, static_cast<float>(collision_sphere_radius));
	collision_shape->setRadius(radius);
	collision_shape->setCollisionMask(camera_collision_mask);
	rebuild_collision_exclude();

	const Vec3 desired_offset = desired_position - target_position;
	const float desired_distance = static_cast<float>(desired_offset.length());

	if (desired_distance <= Consts::EPS)
		return desired_position;

	if (radius <= Consts::EPS)
	{
		if (World::getIntersection(target_position, desired_position,
			static_cast<int>(camera_collision_mask), collision_exclude, collision_hit))
			return collision_hit->getPoint();
		return desired_position;
	}

	auto has_blocking_contact = [&](const Vec3 &position)
	{
		collision_shape->setCenter(position);
		collision_contacts.clear();
		collision_shape->getCollision(collision_contacts, 0.0f);
		for (int i = 0; i < collision_contacts.size(); ++i)
		{
			if (!is_ignored_collision_object(collision_contacts[i]->getObject()))
				return true;
		}
		return false;
	};

	// An arm-only solver cannot recover a pivot embedded in geometry. Collapse
	// conservatively instead of assuming the start of the sweep is collision-free.
	// The pivot itself may still overlap; target/pivot placement must resolve that.
	if (has_blocking_contact(target_position))
		return target_position;

	// Use the same collision volume for the whole arm. A center ray and an endpoint
	// sphere disagree at grazing angles: the ray can alternate between hitting and
	// missing the floor while the sphere remains in contact, producing a large jump.
	const Vec3 sweep_direction = desired_offset / desired_distance;
	int sweep_steps = static_cast<int>(std::ceil(desired_distance / radius));
	sweep_steps = max(1, min(sweep_steps, COLLISION_SWEEP_MAX_STEPS));
	float free_fraction = 0.0f;
	float blocked_fraction = 1.0f;
	bool sweep_blocked = false;

	for (int step = 1; step <= sweep_steps; ++step)
	{
		const float fraction = static_cast<float>(step) / static_cast<float>(sweep_steps);
		const Vec3 sample_position = target_position
			+ sweep_direction * (desired_distance * fraction);
		if (has_blocking_contact(sample_position))
		{
			blocked_fraction = fraction;
			sweep_blocked = true;
			break;
		}
		free_fraction = fraction;
	}

	if (sweep_blocked)
	{
		for (int iteration = 0; iteration < COLLISION_SWEEP_REFINEMENT_ITERATIONS; ++iteration)
		{
			const float fraction = (free_fraction + blocked_fraction) * 0.5f;
			const Vec3 sample_position = target_position
				+ sweep_direction * (desired_distance * fraction);
			if (has_blocking_contact(sample_position))
				blocked_fraction = fraction;
			else
				free_fraction = fraction;
		}
	}
	return target_position + sweep_direction * (desired_distance * free_fraction);
}

Vec3 TestSpringArmCamera::update_collision_smooth_return(const Vec3 &target_position,
	const Vec3 &desired_position, const Vec3 &collision_position, float ifps)
{
	const Vec3 desired_offset = desired_position - target_position;
	const float desired_distance = static_cast<float>(desired_offset.length());
	if (desired_distance <= Consts::EPS)
	{
		current_collision_distance = 0.0f;
		collision_return_velocity = 0.0f;
		collision_return_delay_remaining = 0.0f;
		collision_return_initialized = true;
		return target_position;
	}

	const Vec3 arm_direction = desired_offset / desired_distance;
	const float collision_distance = clamp(
		static_cast<float>(dot(collision_position - target_position, arm_direction)),
		0.0f, desired_distance);

	if (!collision_return_initialized || !enable_camera_collision
		|| !enable_smooth_collision_return)
	{
		current_collision_distance = collision_distance;
		collision_return_velocity = 0.0f;
		collision_return_delay_remaining = 0.0f;
		collision_return_initialized = true;
		return target_position + arm_direction * current_collision_distance;
	}

	if (collision_distance < current_collision_distance)
	{
		// New inward compression wins immediately, even while time is paused.
		current_collision_distance = collision_distance;
		collision_return_velocity = 0.0f;
		collision_return_delay_remaining = max(0.0f,
			static_cast<float>(collision_return_delay));
		return target_position + arm_direction * current_collision_distance;
	}
	if (ifps <= 0.0f)
		return target_position + arm_direction * current_collision_distance;

	float response_dt = ifps;
	if (collision_return_delay_remaining > 0.0f)
	{
		response_dt = max(0.0f, ifps - collision_return_delay_remaining);
		collision_return_delay_remaining = max(0.0f,
			collision_return_delay_remaining - ifps);
		if (response_dt <= 0.0f)
			return target_position + arm_direction * current_collision_distance;
	}

	const float remaining_distance = collision_distance - current_collision_distance;
	if (remaining_distance <= Consts::EPS)
	{
		current_collision_distance = collision_distance;
		collision_return_velocity = 0.0f;
		return target_position + arm_direction * current_collision_distance;
	}

	const float response_time = max(0.0f, static_cast<float>(collision_return_time_95));
	if (response_time > Consts::EPS)
	{
		const float next_distance = spring_step(current_collision_distance, collision_distance,
			collision_return_velocity, response_time, response_dt);
		current_collision_distance = clamp(next_distance,
			current_collision_distance, collision_distance);
		if (current_collision_distance >= collision_distance)
			collision_return_velocity = 0.0f;
	}
	else
	{
		current_collision_distance = collision_distance;
		collision_return_velocity = 0.0f;
	}
	return target_position + arm_direction * current_collision_distance;
}

void TestSpringArmCamera::rebuild_collision_exclude()
{
	collision_exclude.clear();
	collision_ignore_root = nullptr;

	NodePtr current = target.get();
	while (current)
	{
		if (current->isObject())
		{
			collision_ignore_root = current;
			break;
		}
		current = current->getParent();
	}

	if (!collision_ignore_root)
		collision_ignore_root = target.get();
	if (!collision_ignore_root)
		return;

	collision_exclude.append(collision_ignore_root);
	for (int i = 0; i < collision_exclude.size(); ++i)
	{
		const NodePtr excluded_node = collision_exclude[i];
		for (int child_index = 0; child_index < excluded_node->getNumChildren(); ++child_index)
			collision_exclude.append(excluded_node->getChild(child_index));
	}
}

bool TestSpringArmCamera::is_ignored_collision_object(const ObjectPtr &object) const
{
	if (!object || !collision_ignore_root)
		return false;

	NodePtr current = object;
	while (current)
	{
		if (current == collision_ignore_root)
			return true;
		current = current->getParent();
	}
	return false;
}

Vec3 TestSpringArmCamera::get_target_position() const
{
	return target->getWorldTransform() * Vec3(target_offset);
}
