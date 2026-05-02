#include "CharacterMovement.h"

#include "MovementState.h"
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

REGISTER_COMPONENT(CharacterMovement);

void CharacterMovement::init()
{
	FLOGERR(target, "target is not set\n");
	ObjectPtr obj = checked_ptr_cast<Object>(body.get());

	FLOGERR(obj, "can't get object from \"body\"\n");

	// BodyDummy — no physics simulation, just collision detection
	_body = checked_ptr_cast<BodyDummy>(obj->getBody());
	FLOGERR(_body, "can't get BodyDummy from \"body\"\n");

	auto pose = checked_ptr_cast<NodeSkeletonPose>(body->getParent());
	FLOGERR(pose, "can't get NodeSkeletonPose\n");

	_anim = pose->getAnimScript();
	FLOGERR(_anim, "can't get anim\n");

	for (int i = 0; i < _body->getNumShapes(); ++i)
		if (!_shape)
			_shape = checked_ptr_cast<ShapeCapsule>(_body->getShape(i));

	FLOGERR(_shape, "can't get ShapeCapsule from \"body\"\n");

	_ctx.input.init(node);

	_player_ifps = 1.0f / playerFps;
	_slope_cos = Math::cos(slopeLimit * Consts::DEG2RAD);
	_slide_max_cos = Math::cos(slideMaxAngle * Consts::DEG2RAD);
	_escape_slope_cos = Math::cos(escapeSlideAngle * Consts::DEG2RAD);

	_world_transform = obj->getWorldTransform();

	setGravity(Vec3(Physics::getGravity()));

#ifdef DEBUG_MOVEMENT
	Visualizer::setEnabled(true);
#endif

	_ctx.owner = this;
	_states[MovementStateIndex::IDLE] = &_idle_state;
	_states[MovementStateIndex::MOVE] = &_move_state;
	_states[MovementStateIndex::SLIDE] = &_slide_state;
}

void CharacterMovement::update()
{
	float ifps = Game::getIFps() * Physics::getScale();
	_ctx.input.update();

	// set context input values for states
	_ctx.ground_normal = get_ground_normal();
	if (_ctx.ground_normal == vec3_zero)
		_ctx.ground_normal = _up;
	_ctx.character_forward = vec3(_world_transform.getAxisY());
	_ctx.desired_input_direction = compute_desired_input_direction();
	_ctx.is_grounded = _is_grounded;
	_ctx.is_on_steep_slope = _on_steep_slope;
	_ctx.steep_slope_normal = _steep_slope_normal;
	_ctx.max_below_slope_dot = _max_below_slope_dot;

	// #ifdef DEBUG_MOVEMENT
	// Log::message("state: %s\n", _states[_current_state]->name());
	// #endif
	// set default context output values
	_ctx.speed = 0.0f;
	_ctx.turn_speed = 0.0f;
	_ctx.vertical_impulse = 0.0f;
	_ctx.move_direction = _ctx.character_forward;
	_ctx.rotate_target = _ctx.character_forward;
	MovementStateIndex next_state = _states[_current_state]->update(_ctx, ifps);
	if (next_state != MovementStateIndex::NONE)
	{
		_states[_current_state]->onExit(_ctx);
		_current_state = next_state;
		_states[_current_state]->onEnter(_ctx);
	}

	if (_ctx.is_grounded)
		_grounded_flag.stamp();

	if (_states[_current_state]->canJump() && (_ctx.is_grounded || _grounded_flag.isFresh(coyoteTime)) && _ctx.input.consumeJump(jumpBufferTime))
	{
		_grounded_flag.clear();
		_ctx.vertical_impulse = jumpPower / ifps;
		_adaptive_jump_pending = true;
	}

	if (_adaptive_jump_pending && _ctx.input.consumeJumpRelease() && _vertical_speed > adaptiveJumpThreshold * jumpPower)
	{
		_vertical_speed *= 1.0f - adaptiveJumpDamping;
		_adaptive_jump_pending = false;
	}

	Vec3 desired_horizontal = Vec3(_ctx.move_direction * _ctx.speed);
	if (_is_grounded)
	{
		_horizontal_velocity = desired_horizontal;
	}
	else if (length2(desired_horizontal) > Consts::EPS)
	{
		// Airborne with input — gradually steer toward the desired direction
		// without snapping. Preserves momentum from the previous state (e.g.,
		// jumping out of a slide).
		float t = saturate(airControl * ifps);
		_horizontal_velocity += (desired_horizontal - _horizontal_velocity) * Scalar(t);
	}
	// else: airborne with no input — keep current momentum.

	float slope_cos = dot(_ctx.move_direction, _up);
	float move_coeff = 1.0f - slope_cos;

	_world_transform = target->getWorldTransform();

#ifdef DEBUG_MOVEMENT
	auto p0 = _world_transform.getTranslate();
	Visualizer::renderVector(p0, p0 + Vec3(_horizontal_velocity), vec4_green);
	Visualizer::renderVector(p0, p0 + Vec3(_ctx.move_direction), vec4_green);
	Visualizer::renderVector(p0, p0 + Vec3(_up), vec4_white);
	Visualizer::renderVector(p0, p0 + Vec3(_gravity_direction), vec4_blue);
	Visualizer::renderVector(p0, p0 + Vec3(_ctx.desired_input_direction), vec4_white);

	Visualizer::renderMessage3D(p0 + Vec3_up * 1.5, vec3_zero, "_is_grounded", _is_grounded ? vec4_green : vec4_red);
	Visualizer::renderMessage3D(p0 + Vec3_up * 2, vec3_zero, String::format("_vertical_speed: %f", _vertical_speed), _vertical_speed != 0.0f ? vec4_green : vec4_red);
#endif

	float update_time = ifps;
	_is_grounded = false;
	_walkable_grounded = false;
	_on_steep_slope = false;
	_max_below_slope_dot = 0.0f;
	_steep_slope_normal = vec3_zero;
	while (update_time > 0.0f)
	{
		float adaptive_time_step = min(update_time, _player_ifps);
		update_time -= adaptive_time_step;

		_vertical_speed += _ctx.vertical_impulse * adaptive_time_step;
		if (!_is_grounded)
			_vertical_speed -= _gravity_amount * adaptive_time_step;

		Vec3 verticale_velocity = Vec3(_up * _vertical_speed);

		mul(_world_transform, translate((_horizontal_velocity * move_coeff + verticale_velocity) * adaptive_time_step), _world_transform);

		Vec3 saved_velocity = _horizontal_velocity;
		resolve_collisions(adaptive_time_step);
		_horizontal_velocity = saved_velocity;

		rotate(_ctx.rotate_target, _ctx.turn_speed, adaptive_time_step);
	}

	// Walkable wins over slidable: if we touch a walkable surface anywhere,
	// the character is treated as standing on it, not sliding.
	if (_walkable_grounded)
		_walkable_flag.stamp();

	// Hysteresis: if we just had a walkable contact, suppress slide entry.
	// Otherwise grazing a steep slope while walking on flat would chatter
	// SLIDE↔MOVE (capsule's lower hemisphere intermittently loses the flat
	// contact while still touching the slope).
	bool slide_suppressed = _walkable_flag.isFresh(slideEntryDelay);
	_on_steep_slope = !_walkable_grounded
				   && (_steep_slope_normal != vec3_zero)
				   && !slide_suppressed;
	if (_on_steep_slope)
		_steep_slope_normal = normalizeValid(_steep_slope_normal);
	else
		_steep_slope_normal = _up;

	target->setWorldTransform(_world_transform);
	body->setWorldTransform(target->getWorldTransform());


	//%%%%%%%%%%%%%%%%%%% Anim %%%%%%%%%%%%%%%
	{
		bool is_sliding = _current_state == MovementStateIndex::SLIDE;
		bool is_moving = !is_sliding && !compare(_ctx.speed, 0.0f);
		bool is_spinting = !is_sliding && abs(_ctx.speed) > runSpeed;
		// Smooth out micro losses of ground contact at surface seams: while
		// the player is descending/standing and was on ground a moment ago,
		// keep the animation in "grounded" state. Cleared on jump (the flag
		// is reset there), so falling/jumping still shows airborne anim.
		bool is_grounded = _is_grounded
						|| (_vertical_speed <= 0.0f && _grounded_flag.isFresh(groundedAnimCoyote));
		bool is_idle = !is_moving && !is_spinting && !is_sliding && is_grounded;
		_anim->setParamBool("is_moving", is_moving);
		_anim->setParamBool("is_sprinting", is_spinting);
		_anim->setParamBool("is_grounded", is_grounded);
		_anim->setParamBool("is_idle", is_idle);
		_anim->setParamBool("is_sliding", is_sliding);
		_anim->setParamFloat("rand_float", Game::getRandomFloat(0.0f, 1.0f));
	}
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

void CharacterMovement::applyVerticalBounce(float speed)
{
	if (speed > _vertical_speed)
		_vertical_speed = speed;
	// Cancel adaptive jump damping — the bounce is intentional, releasing jump
	// shouldn't immediately scrub the speed we just gave.
	_adaptive_jump_pending = false;
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

vec3 CharacterMovement::compute_desired_input_direction() const
{
	vec2 move_input = _ctx.input.getMoveInput();

	vec3 view_dir = Game::getPlayer()->getViewDirection();
	vec3 forward_dir = normalize(view_dir - _up * dot(view_dir, _up));
	vec3 right_dir = normalize(cross(forward_dir, _up));

	return normalizeValid(forward_dir * move_input.y + right_dir * move_input.x);
}

vec3 CharacterMovement::project_forward_on_ground(const vec3 &ground_normal)
{
	vec3 move_dir = vec3(_world_transform.getAxisY());
	float up_normal_cos = dot(_up, ground_normal);
	if (Math::abs(up_normal_cos) >= Consts::EPS)
		move_dir -= _up * (dot(move_dir, ground_normal) / up_normal_cos);

	move_dir.normalize();
	return move_dir;
}

void CharacterMovement::resolve_collisions(float ifps)
{
	for (int iter = 0; iter < collisionIterations; ++iter)
	{
		_body->setTransform(_world_transform);

		_shape->getCollision(_contacts, ifps);
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
			bool is_slidable = is_below && !is_walkable && slope_dot > _slide_max_cos;
			bool is_ground_contact = is_walkable || is_slidable;

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

			if (is_ground_contact)
			{
				_is_grounded = true;
				_max_below_slope_dot = max(_max_below_slope_dot, slope_dot);

				// Vertical speed is cancelled by the surface — let slide motion
				// (or just standing) take over from there.
				if (_vertical_speed < 0.0f)
					_vertical_speed = 0.0f;

				if (is_walkable)
				{
					_walkable_grounded = true;
					if (_vertical_speed > 0.0f)
						_vertical_speed *= 0.3f;
				}
				else
				{
					_steep_slope_normal += normal;
				}
			}
		}

		_world_transform.setColumn3(3, _world_transform.getTranslate() + Vec3(pos_offset));
	}
}

void CharacterMovement::rotate(const vec3 &direction, float turn_speed, float ifps)
{
	// if character's up and _up are parallel, then it should be ok to skip normalize().
	// we assume they are parallel
	vec3 forward = _world_transform.getRotate() * vec3_forward;
	// forward = normalize(forward - _up * dot(forward, _up));

	float angle = Math::atan2(
					  dot(cross(forward, direction), _up),
					  dot(forward, direction)) *
				  Consts::RAD2DEG;

	float max_step = turn_speed * ifps;

	float step = clamp(angle, -max_step, max_step);
	quat delta_rot = quat(_up, step);
	quat current_rot = _world_transform.getRotate();

	_world_transform = Mat4(delta_rot * current_rot, _world_transform.getTranslate());
}
