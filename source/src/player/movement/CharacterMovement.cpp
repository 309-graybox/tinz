#include "CharacterMovement.h"

#include "utils/Utils.h"

#include <UnigineGame.h>
#include <UnigineMathLib.h>
#include <UnigineMathLibCommon.h>
#include <UnigineMathLibVec3.h>
#include <UniginePhysics.h>
#include <UnigineVisualizer.h>
#include <UnigineWorld.h>

using namespace Unigine;
using namespace Math;

#define DEBUG_MOVEMENT
// Keeping both methods for now in case of future modifications.
// We can remove one later.
#define FIRST_ROTATE

REGISTER_COMPONENT(CharacterMovement);

void CharacterMovement::init()
{
	FLOGERR(target, "target is not set\n");
	ObjectPtr obj = checked_ptr_cast<Object>(body.get());

	FLOGERR(obj, "can't get object from \"body\"\n");

	// BodyDummy — no physics simulation, just collision detection
	_body = checked_ptr_cast<BodyDummy>(obj->getBody());
	FLOGERR(_body, "can't get BodyDummy from \"body\"\n");

	for (int i = 0; i < _body->getNumShapes(); ++i)
		if (!_shape)
			_shape = checked_ptr_cast<ShapeCapsule>(_body->getShape(i));

	FLOGERR(_shape, "can't get ShapeCapsule from \"body\"\n");

	_input.init(node);

	_player_ifps = 1.0f / playerFps;
	_shape_height = _shape->getHeight();
	_slope_cos = Math::cos(slopeLimit * Consts::DEG2RAD);
	_sharp_turn_cos = Math::cos(sharpTurnAngleThreshold);
	_sharp_turn_cos = Math::cos(sharpTurnAngleThreshold * Consts::DEG2RAD);
	turnSpeed = turnSpeed * Consts::DEG2RAD;
	sprintTurnSpeed = sprintTurnSpeed * Consts::DEG2RAD;

	_world_transform = obj->getWorldTransform();

	setGravity(Vec3(Physics::getGravity()));

#ifdef DEBUG_MOVEMENT
	Visualizer::setEnabled(true);
#endif
}
void CharacterMovement::update()
{
	float ifps = Game::getIFps() * Physics::getScale();

	auto ground_normal = get_ground_normal();
	if (ground_normal == vec3_zero)
		ground_normal = vec3_up;
	_input.update();

	// jump just for now
	_vertical_move = 0.0f;
	if (_input.consumeJump() && _is_grounded)
	{
		_vertical_move = jumpPower / ifps;
		// crouch just for now
	} else if (_input.isCrouching() && !_is_grounded)
	{
		_vertical_move = -jumpPower / ifps;
	}

	float speed = _input.isSprinting() ? sprintSpeed
									   : _input.isWalking() ? walkSpeed
															: runSpeed;
	float turn_speed = _input.isSprinting() ? sprintTurnSpeed
											: turnSpeed;

	vec3 move_direction = vec3(_world_transform.getAxisY());
	
	vec3 desired_move_direction = _input.getDesiredDirection();
	// vec3 move_direction_up = vec3(_world_transform.getAxisZ());
	// desired_move_direction -= move_direction_up * dot(desired_move_direction, move_direction_up);
	// desired_move_direction.normalize();
	vec3 desired_move_direction;
	vec3 move_direction = calculate_move_direction(ground_normal, desired_move_direction);

	vec3 character_forward = vec3(_world_transform.getAxisY());

	// +1 -> 0 -> -1
	float cos_move_direction = dot(desired_move_direction, move_direction);
	// cos: +1 -> 0 -> -1
	float cos_move_direction = dot(desired_move_direction, character_forward);

	if (cos_move_direction > _sharp_turn_cos)
	{
		
	}


	float slope_cos = dot(move_direction, _up);
	float move_coeff = 1.0f - slope_cos;

	_world_transform = target->getWorldTransform();
	_horizontal_velocity = Vec3(move_direction * speed * toFloat(_input.isInputMoving()));

#ifdef DEBUG_MOVEMENT
	auto p0 = _world_transform.getTranslate();
	// Visualizer::renderVector(p0, p0 + Vec3(target_velocity), vec4_white);
	Visualizer::renderVector(p0, p0 + Vec3(_horizontal_velocity), vec4_green);
	Visualizer::renderVector(p0, p0 + Vec3(move_direction), vec4_green);
	Visualizer::renderVector(p0, p0 + Vec3(_up), vec4_white);
	Visualizer::renderVector(p0, p0 + Vec3(_gravity_direction), vec4_blue);
	
	Visualizer::renderMessage3D(p0 + Vec3_up * 1.5, vec3_zero, "_is_grounded", _is_grounded ? vec4_green : vec4_red);
	Visualizer::renderMessage3D(p0 + Vec3_up * 2, vec3_zero, String::format("_vertical_speed: %f", _vertical_speed), _vertical_speed != 0.0f ? vec4_green : vec4_red);
#endif

	float update_time = ifps;
	_is_grounded = false;
	while (update_time > 0.0f)
	{
		float adaptive_time_step = min(update_time, _player_ifps);
		update_time -= adaptive_time_step;

		// update_velocity(adaptive_time_step);

		_vertical_speed += _vertical_move * adaptive_time_step;
		if (!_is_grounded)
			_vertical_speed -= _gravity_amount * adaptive_time_step;

		Vec3 verticale_velocity = Vec3(_up * _vertical_speed);

		mul(_world_transform, translate((_horizontal_velocity * move_coeff + verticale_velocity) * adaptive_time_step), _world_transform);

		Vec3 saved_velocity = _horizontal_velocity;
		resolve_collisions(adaptive_time_step);
		_horizontal_velocity = saved_velocity;

		rotate(desired_move_direction, turn_speed, speed, adaptive_time_step);
	}

	target->setWorldTransform(_world_transform);
	body->setWorldTransform(target->getWorldTransform());
}
void CharacterMovement::shutdown()
{
	// Visualizer::setEnabled(false);
}
void CharacterMovement::setGravity(const Unigine::Math::Vec3 &gravity)
{
	_gravity_amount = gravity.length();
	_gravity_direction = vec3(normalizeValid(gravity));
	_up = -_gravity_direction;
}

Unigine::Math::vec3 CharacterMovement::get_ground_normal() const
{
	if (!_is_grounded)
		return vec3_up;

	vec3 normal;

	Vec3 pos = _shape->getBottomCap();
	float radius = _shape->getRadius();
	Vec3 down_ray = Vec3(_gravity_direction * radius * groundCheckRaysLength);

	auto t = _shape->getTransform();
	Vec3 axes[5] = {Vec3_zero, t.getAxisX(), -t.getAxisX(), t.getAxisY(), -t.getAxisY()};

	WorldIntersectionNormalPtr hit_normal = WorldIntersectionNormal::create();
	for (int i = 0; i < 5; ++i)
	{
		Vec3 p0 = pos + axes[i] * radius;
		auto object = World::getIntersection(p0, p0 + down_ray, groundCheckIntersectionMask, {body}, hit_normal);
		
#ifdef DEBUG_MOVEMENT
		Visualizer::renderVector(p0, p0 + down_ray, vec4_blue, 0.01f);
#endif
		if (object)
		{
#ifdef DEBUG_MOVEMENT
			Visualizer::renderPoint3D(hit_normal->getPoint(), 0.01f, vec4_blue, false, 0.0f, false);
#endif
			normal += hit_normal->getNormal();
		}
	}

	return normal;
}

vec3 CharacterMovement::calculate_move_direction(const vec3 &ground_normal, vec3 &ret_desired_direction)
{
	vec2 move_input = _input.getMoveInput();

	vec3 view_dir = Game::getPlayer()->getViewDirection();
	vec3 forward_dir = normalize(view_dir - _up * dot(view_dir, _up));
	vec3 right_dir = normalize(cross(forward_dir, _up));
	// vec3 move_dir = forward_dir * move_input.y + right_dir * move_input.x;
	vec3 move_dir = vec3(_world_transform.getAxisY());
	ret_desired_direction = forward_dir * move_input.y + right_dir * move_input.x;
	
#ifdef DEBUG_MOVEMENT
	Vec3 p0 = _world_transform.getTranslate();
	Visualizer::renderMessage3D(p0 + Vec3_up * 1.75, vec3_zero, String::format("forward: %f, right: %f", move_input.y, move_input.x), vec4_green);
	Visualizer::renderMessage3D(p0 + Vec3_up * 2.25, vec3_zero, String::format("x: %f, y: %f", ret_desired_direction.y, ret_desired_direction.x), vec4_green);
#endif

	float up_normal_cos = dot(_up, ground_normal);
	if (Math::abs(up_normal_cos) >= Consts::EPS)
		move_dir -= _up * (dot(move_dir, ground_normal) / up_normal_cos);

	ret_desired_direction.normalize();
	move_dir.normalize();
	return move_dir;
}

void CharacterMovement::update_velocity(float delta)
{
	// _velocity += _vertical_move * delta;
	// if (!_is_grounded)
		// _velocity += gravity * delta;
}

void CharacterMovement::resolve_collisions(float delta)
{
	for (int iter = 0; iter < collisionIterations; ++iter)
	{
		_body->setTransform(_world_transform);

		_shape->getCollision(_contacts, delta);
		if (_contacts.size() == 0)
			break;

		int count = Math::min(_contacts.size(), 16);
		float icount = 1.0f / toFloat(count);

		vec3 pos_offset = vec3_zero;
		Vec3 bottom_cap = _shape->getBottomCap();

		for (int i = 0; i < count; ++i)
		{
			const ConstShapeContactPtr &contact = _contacts[i];
			vec3 normal = contact->getNormal();
			float depth = contact->getDepth();

			Vec3 contact_point = contact->getPoint();
			
			float slope_dot = dot(normal, _up);
			bool is_below = dot(contact_point - bottom_cap, Vec3(_up)) < 0.0f;
			bool is_walkable = is_below && slope_dot > _slope_cos;

			pos_offset += is_walkable
							// ? up * dot(normal, up) * depth * icount
							? _up * depth * icount
							: normal * depth * icount;

			float normal_speed = toFloat(dot(Vec3(normal), _horizontal_velocity));
			if (normal_speed < 0.0f)
				_horizontal_velocity -= Vec3(normal) * normal_speed;

#ifdef DEBUG_MOVEMENT
			Visualizer::renderPoint3D(contact_point, 0.01f, vec4_red);
			Visualizer::renderVector(contact_point, contact_point + Vec3(normal), vec4_black);
#endif

			if (is_walkable)
			{
				_is_grounded = true;
				if (_vertical_speed < 0.0f)
					_vertical_speed = 0.0f;
			}

			// Ceiling detection
			if (dot(normal, vec3_down) > _slope_cos)
			{
				if (_vertical_speed > 0.0f)
					_vertical_speed = 0.0f;
			}
		}

		_world_transform.setColumn3(3, _world_transform.getTranslate() + Vec3(pos_offset));
	}
}

void CharacterMovement::rotate(const vec3 &direction, float turn_speed, float speed, float delta)
{
// keeping both methods for now in case of future modifications
// #undef FIRST_ROTATE
#ifdef FIRST_ROTATE
	float len2 = direction.length2();
	if (compare(len2, 0.0f))
		return;

	vec3 right = normalize(cross(direction, _up));
	vec3 up = normalize(cross(right, direction));

	quat target_rot = quat(right, direction, up);
	quat current_rot = _world_transform.getRotate();

#ifdef DEBUG_MOVEMENT
	auto p0 = _world_transform.getTranslate() + Vec3_up * 2.0f;
	Visualizer::renderVector(p0, p0 + Vec3(direction), vec4_green);
	Visualizer::renderVector(p0, p0 + Vec3(right), vec4_red);
	Visualizer::renderVector(p0, p0 + Vec3(up), vec4_blue);
#endif

	float t = clamp(turn_speed * delta, 0.0f, 1.0f);
	_world_transform = Mat4(slerp(current_rot, target_rot, t), _world_transform.getTranslate());
#else
	// if character's up and _up are parallel, then it should be ok to skip normalize().
	// we assume they are parallel
	vec3 forward = _world_transform.getRotate() * vec3_forward;
	// forward = normalize(forward - _up * dot(forward, _up));

	float angle = Math::atan2(
		dot(cross(forward, direction), _up),
		dot(forward, direction)) * Consts::RAD2DEG;

	quat delta_rot = quat(_up, angle * turn_speed * delta);
	quat current_rot = _world_transform.getRotate();

	_world_transform = Mat4(delta_rot * current_rot, _world_transform.getTranslate());
#endif
}