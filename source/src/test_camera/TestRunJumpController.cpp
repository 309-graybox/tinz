#include "TestRunJumpController.h"

#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineMathLib.h>

using namespace Unigine;
using namespace Unigine::Math;

REGISTER_COMPONENT(TestRunJumpController)

void TestRunJumpController::init()
{
	object = checked_ptr_cast<Object>(node);
	if (!object)
		return;

	body = checked_ptr_cast<BodyDummy>(object->getBody());
	if (!body)
		return;

	for (int i = 0; i < body->getNumShapes(); ++i)
	{
		shape = checked_ptr_cast<ShapeCapsule>(body->getShape(i));
		if (shape)
			break;
	}
	if (!shape)
		return;

	shape->setCollisionMask(movement_collision_mask);
	world_transform = object->getWorldTransform();
	ground_normal_cos = cos(static_cast<float>(max_ground_angle) * Consts::DEG2RAD);
	controller_ready = true;
}

void TestRunJumpController::update()
{
	if (!controller_ready)
		return;

	const float ifps = Game::getIFps();
	if (ifps <= 0.0f)
		return;

	const vec3 gravity = Physics::getGravity();
	gravity_amount = gravity.length();
	if (gravity_amount > Consts::EPS)
		up = -gravity / gravity_amount;

	const bool accepts_input = Input::isMouseCursorHide();
	const vec3 move_direction = accepts_input ? read_move_direction() : vec3_zero;
	update_horizontal_velocity(move_direction, ifps);

	if (move_direction.length2() > Consts::EPS)
		rotate_towards(move_direction, ifps);

	if (accepts_input && can_jump && Input::isKeyDown(Input::KEY_SPACE))
	{
		vertical_speed = max(0.0f, static_cast<float>(jump_speed));
		grounded = false;
		can_jump = false;
		adaptive_jump_pending = true;
	}

	if (adaptive_jump_pending && accepts_input && Input::isKeyUp(Input::KEY_SPACE))
	{
		const float threshold = max(0.0f, static_cast<float>(adaptive_jump_threshold))
			* max(0.0f, static_cast<float>(jump_speed));
		if (vertical_speed > threshold)
		{
			const float damping_fraction = clamp(static_cast<float>(adaptive_jump_damping), 0.0f, 1.0f);
			vertical_speed *= 1.0f - damping_fraction;
		}
		adaptive_jump_pending = false;
	}

	vertical_speed -= gravity_amount * ifps;
	const Vec3 velocity = Vec3(horizontal_velocity + up * vertical_speed);
	world_transform.setColumn3(3, world_transform.getTranslate() + velocity * ifps);

	resolve_collisions(ifps);

	object->setWorldTransform(world_transform);
	body->setTransform(world_transform);
}

vec3 TestRunJumpController::read_move_direction() const
{
	float forward_input = 0.0f;
	float right_input = 0.0f;

	forward_input += Input::isKeyPressed(Input::KEY_W) ? 1.0f : 0.0f;
	forward_input -= Input::isKeyPressed(Input::KEY_S) ? 1.0f : 0.0f;
	right_input += Input::isKeyPressed(Input::KEY_D) ? 1.0f : 0.0f;
	right_input -= Input::isKeyPressed(Input::KEY_A) ? 1.0f : 0.0f;

	vec3 forward = vec3(world_transform.getRotate() * vec3_forward);
	if (const PlayerPtr &player = Game::getPlayer())
		forward = player->getViewDirection();

	forward -= up * dot(forward, up);
	if (forward.length2() < Consts::EPS)
		forward = vec3(world_transform.getRotate() * vec3_forward);
	forward.normalizeValid();

	vec3 right = cross(forward, up);
	right.normalizeValid();

	vec3 direction = forward * forward_input + right * right_input;
	direction.normalizeValid();
	return direction;
}

void TestRunJumpController::update_horizontal_velocity(const vec3 &move_direction, float ifps)
{
	if (move_direction.length2() > Consts::EPS)
	{
		const vec3 target_velocity = move_direction * max(0.0f, static_cast<float>(run_speed));
		move_towards(horizontal_velocity, target_velocity,
			max(0.0f, static_cast<float>(acceleration)) * ifps);
	}
	else
	{
		move_towards(horizontal_velocity, vec3_zero,
			max(0.0f, static_cast<float>(damping)) * ifps);
	}
}

void TestRunJumpController::rotate_towards(const vec3 &direction, float ifps)
{
	vec3 forward = vec3(world_transform.getRotate() * vec3_forward);
	forward -= up * dot(forward, up);
	forward.normalizeValid();

	const float angle = atan2(dot(cross(forward, direction), up), dot(forward, direction))
		* Consts::RAD2DEG;
	const float max_step = max(0.0f, static_cast<float>(turn_speed)) * ifps;
	const float step = clamp(angle, -max_step, max_step);

	const quat rotation = quat(up, step) * world_transform.getRotate();
	world_transform = Mat4(rotation, world_transform.getTranslate());
}

void TestRunJumpController::resolve_collisions(float ifps)
{
	grounded = false;
	can_jump = false;

	const int iterations = max(1, static_cast<int>(collision_iterations));
	for (int iteration = 0; iteration < iterations; ++iteration)
	{
		body->setTransform(world_transform);
		contacts.clear();
		shape->getCollision(contacts, ifps);
		if (contacts.size() == 0)
			break;

		const float inverse_contact_count = 1.0f / static_cast<float>(contacts.size());
		vec3 position_offset = vec3_zero;
		const Vec3 bottom_cap = shape->getBottomCap();

		for (int i = 0; i < contacts.size(); ++i)
		{
			const ShapeContactPtr &contact = contacts[i];
			const vec3 normal = contact->getNormal();
			const float depth = contact->getDepth();
			const bool below_capsule = dot(contact->getPoint() - bottom_cap, Vec3(up)) < 0.0;
			const bool walkable_ground = below_capsule && dot(normal, up) >= ground_normal_cos;

			position_offset += (walkable_ground ? up : normal) * depth * inverse_contact_count;

			const float horizontal_normal_speed = dot(horizontal_velocity, normal);
			if (horizontal_normal_speed < 0.0f)
				horizontal_velocity -= normal * horizontal_normal_speed;

			if (walkable_ground && vertical_speed <= 0.0f)
			{
				grounded = true;
				vertical_speed = 0.0f;

				const ObjectPtr contact_object = contact->getObject();
				const int surface = contact->getSurface();
				if (contact_object && surface >= 0
					&& (contact_object->getIntersectionMask(surface)
						& static_cast<int>(jump_surface_intersection_mask)) != 0)
				{
					can_jump = true;
				}
			}
			else if (dot(normal, up) < 0.0f && vertical_speed > 0.0f)
			{
				vertical_speed = 0.0f;
				adaptive_jump_pending = false;
			}
		}

		world_transform.setColumn3(3,
			world_transform.getTranslate() + Vec3(position_offset));
	}

	if (grounded)
		adaptive_jump_pending = false;
}

void TestRunJumpController::move_towards(vec3 &current, const vec3 &target, float max_delta)
{
	const vec3 delta = target - current;
	const float distance = delta.length();
	if (distance <= max_delta || distance < Consts::EPS)
		current = target;
	else
		current += delta * (max_delta / distance);
}
