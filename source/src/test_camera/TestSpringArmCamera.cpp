#include "TestSpringArmCamera.h"

#include <UnigineInput.h>
#include <UnigineMathLib.h>
#include <UnigineWorld.h>

using namespace Unigine;
using namespace Unigine::Math;

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
		return;
	}

	if (!angles_initialized)
		initialize_angles_from_camera();

	if (camera->isMainPlayer() && Input::isMouseCursorHide())
	{
		const ivec2 mouse_delta = Input::getMouseDeltaRaw();
		const float sensitivity = mouse_sensitivity;

		yaw += static_cast<float>(mouse_delta.x) * sensitivity;

		float pitch_delta = static_cast<float>(mouse_delta.y) * sensitivity;
		if (invert_pitch)
			pitch_delta = -pitch_delta;
		pitch += pitch_delta;

		pitch = clamp(pitch, static_cast<float>(min_pitch), static_cast<float>(max_pitch));
		while (yaw > 180.0f)
			yaw -= 360.0f;
		while (yaw < -180.0f)
			yaw += 360.0f;
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
}

void TestSpringArmCamera::apply_camera_transform()
{
	vec3 camera_direction = vec3_forward * rotateZ(yaw);
	camera_direction = camera_direction * rotate(cross(camera_direction, vec3_up), pitch);
	camera_direction.normalize();

	const float distance = max(0.0f, static_cast<float>(arm_length));
	const Vec3 target_position = get_target_position();
	const Vec3 desired_position = target_position - Vec3(camera_direction * distance);
	const Vec3 camera_position = resolve_collision(target_position, desired_position);

	vec3 look_direction(target_position - camera_position);
	if (look_direction.length2() < 1e-8f)
		look_direction = camera_direction;
	look_direction.normalizeValid();

	camera->setWorldDirection(look_direction, vec3_up);
	camera->setWorldPosition(camera_position);
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

	Vec3 resolved_position = desired_position;
	const Vec3 desired_offset = desired_position - target_position;
	const float desired_distance = static_cast<float>(desired_offset.length());

	if (desired_distance > Consts::EPS)
	{
		const vec3 outward_direction = vec3(desired_offset / desired_distance);
		if (World::getIntersection(target_position, desired_position,
			static_cast<int>(camera_collision_mask), collision_exclude, collision_hit))
		{
			const float hit_distance = static_cast<float>(
				(collision_hit->getPoint() - target_position).length());
			const float normal_alignment = abs(dot(outward_direction, collision_hit->getNormal()));
			const float clearance = radius / max(normal_alignment, 0.1f);
			const float collision_distance = clamp(hit_distance - clearance, 0.0f, desired_distance);
			resolved_position = target_position + Vec3(outward_direction * collision_distance);
		}
	}

	for (int iteration = 0; iteration < COLLISION_RESOLUTION_ITERATIONS; ++iteration)
	{
		collision_shape->setCenter(resolved_position);
		collision_contacts.clear();
		collision_shape->getCollision(collision_contacts, 0.0f);
		if (collision_contacts.size() == 0)
			break;

		vec3 correction = vec3_zero;
		int valid_contact_count = 0;
		for (int i = 0; i < collision_contacts.size(); ++i)
		{
			const ShapeContactPtr &contact = collision_contacts[i];
			if (is_ignored_collision_object(contact->getObject()))
				continue;

			correction += contact->getNormal() * contact->getDepth();
			++valid_contact_count;
		}
		if (valid_contact_count == 0)
			break;

		correction /= static_cast<float>(valid_contact_count);
		resolved_position += Vec3(correction);
	}

	const Vec3 resolved_offset = resolved_position - target_position;
	const float resolved_distance = static_cast<float>(resolved_offset.length());
	if (resolved_distance > desired_distance && resolved_distance > Consts::EPS)
	{
		resolved_position = target_position
			+ resolved_offset * (desired_distance / resolved_distance);
	}

	collision_shape->setCenter(resolved_position);
	return resolved_position;
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
