#include "CharacterMovement.h"

#include "MovementState.h"
#include "components/Entity.h"
#include "utils/Utils.h"
#include "tuning/DebugTuning.h"

#include <UnigineGame.h>
#include <UnigineLog.h>
#include <UnigineMathLib.h>
#include <UnigineMathLibCommon.h>
#include <UnigineMathLibVec3.h>
#include <UniginePhysics.h>
#include <UnigineVisualizer.h>
#include <UnigineWorld.h>

#define SET_ANIM_BOOL_PARAM(Script, Param, Val)                                    \
	{                                                                              \
		const char *param_name = Param;                                            \
		bool v = Script->getParamBool(param_name);                                 \
		bool nv = Val;                                                             \
		if (v != nv)                                                               \
		{                                                                          \
			Script->setParamBool(Param, nv);                                       \
			if (DebugTuning::get()->log_anim_param_on_change)                      \
			{                                                                      \
				Log::message("Anim param '%s' changed: %i -> %i\n", Param, v, nv); \
			}                                                                      \
		}                                                                          \
	}

using namespace Unigine;
using namespace Math;

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
	{
		if (!_shape)
			_shape = checked_ptr_cast<ShapeCapsule>(_body->getShape(i));
	}

	FLOGERR(_shape, "can't get ShapeCapsule from \"body\"\n");

	_shape_height = _shape->getHeight();

	_ctx.input.init(node);

	_player_ifps = 1.0f / playerFps;
	_slope_cos = Math::cos(slopeLimit * Consts::DEG2RAD);
	_slide_max_cos = Math::cos(slideMaxAngle * Consts::DEG2RAD);
	_escape_slope_cos = Math::cos(escapeSlideAngle * Consts::DEG2RAD);

	_world_transform = obj->getWorldTransform();

	setGravity(Vec3(Physics::getGravity()));

	_ctx.owner = this;
	_states[MovementStateIndex::IDLE] = &_idle_state;
	_states[MovementStateIndex::MOVE] = &_move_state;
	_states[MovementStateIndex::SLIDE] = &_slide_state;
	_damage_knockback_velocity = Vec3_zero;
	_damage_knockback_timer = 0.0f;
	_damage_knockback_duration = 0.0f;
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

	// set default context output values
	_ctx.speed = 0.0f;
	_ctx.turn_responsiveness = 0.0f;
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
	} else if (length2(desired_horizontal) > Consts::EPS)
	{
		// Airborne with input — gradually steer toward the desired direction
		// without snapping. Preserves momentum from the previous state (e.g.,
		// jumping out of a slide).
		float t = saturate(airControl * ifps);
		_horizontal_velocity += (desired_horizontal - _horizontal_velocity) * Scalar(t);
	}
	// else: airborne with no input — keep current momentum.

	Vec3 controlled_horizontal_velocity = _horizontal_velocity;
	if (_damage_knockback_timer > 0.0f && _damage_knockback_duration > Consts::EPS)
	{
		const float weight = saturate(_damage_knockback_timer / _damage_knockback_duration);
		_horizontal_velocity += _damage_knockback_velocity * Scalar(weight);
	}

	float slope_cos = dot(_ctx.move_direction, _up);
	float move_coeff = 1.0f - slope_cos;

	_world_transform = target->getWorldTransform();

	if (DebugTuning::get()->show_movement_base)
	{
		auto p0 = _world_transform.getTranslate();
		Visualizer::renderVector(p0, p0 + Vec3(_horizontal_velocity), vec4_green);
		Visualizer::renderVector(p0, p0 + Vec3(_ctx.move_direction), vec4_green);
		Visualizer::renderVector(p0, p0 + Vec3(_up), vec4_white);
		Visualizer::renderVector(p0, p0 + Vec3(_gravity_direction), vec4_blue);
		Visualizer::renderVector(p0, p0 + Vec3(_ctx.desired_input_direction), vec4_white);

		Visualizer::renderMessage3D(p0 + Vec3_up * 1.5, vec3_zero, "_is_grounded", _is_grounded ? vec4_green : vec4_red);
		Visualizer::renderMessage3D(p0 + Vec3_up * 2, vec3_zero, String::format("_vertical_speed: %f", _vertical_speed), _vertical_speed != 0.0f ? vec4_green : vec4_red);
	}

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
		// Suppress gravity while auto-stepping or descending: vertical motion
		// is driven explicitly, and the player is treated as grounded throughout.
		if (!_is_grounded && !_climbing && !_descending)
			_vertical_speed -= _gravity_amount * adaptive_time_step;

		// Apply gradual vertical lift before motion. Capsule rises in place
		// until it clears the obstacle; horizontal motion follows naturally.
		if (_climbing)
		{
			Scalar current_h = dot(_world_transform.getTranslate(), Vec3(_up));
			Scalar remaining = Scalar(_climb_target_height) - current_h;
			if (remaining > Scalar(0.0))
			{
				Scalar lift_amt = Scalar(stepClimbSpeed) * Scalar(adaptive_time_step);
				Scalar lift = remaining < lift_amt ? remaining : lift_amt;
				_world_transform.setColumn3(3, _world_transform.getTranslate() + Vec3(_up) * lift);
			}
			_vertical_speed = 0.0f;
		}

		// Apply gradual vertical drop before motion. Mirror of climb but
		// downward — pulls capsule toward a lower walkable surface (descending
		// stairs, stepping off curbs) without the visual "jump" of an instant
		// snap.
		if (_descending)
		{
			Scalar current_h = dot(_world_transform.getTranslate(), Vec3(_up));
			Scalar remaining = current_h - Scalar(_descent_target_height);
			if (remaining > Scalar(0.0))
			{
				Scalar drop_amt = Scalar(stepClimbSpeed) * Scalar(adaptive_time_step);
				Scalar drop = remaining < drop_amt ? remaining : drop_amt;
				_world_transform.setColumn3(3, _world_transform.getTranslate() - Vec3(_up) * drop);
			}
			_vertical_speed = 0.0f;
		}

		Vec3 verticale_velocity = Vec3(_up * _vertical_speed);

		Mat4 pre_motion_transform = _world_transform;
		Vec3 horiz_step = _horizontal_velocity * Scalar(move_coeff) * Scalar(adaptive_time_step);

		mul(_world_transform, translate((_horizontal_velocity * move_coeff + verticale_velocity) * adaptive_time_step), _world_transform);

		Vec3 saved_velocity = _horizontal_velocity;
		_hit_wall = false;
		_hit_wall_normal = vec3_zero;
		resolve_collisions(adaptive_time_step);
		_horizontal_velocity = saved_velocity;

		if (_walkable_grounded || _hit_wall)
			_walkable_flag.stamp();

		// Update climbing state. Exit when we're on a walkable surface, hit
		// the target height, or hit the failsafe timeout (e.g. wedged under
		// a ceiling so lift can't make progress).
		if (_climbing)
		{
			_climb_time += adaptive_time_step;
			Scalar current_h = dot(_world_transform.getTranslate(), Vec3(_up));
			bool target_reached = current_h >= Scalar(_climb_target_height) - Scalar(0.005);
			bool timeout = _climb_time > 1.0f;
			if (_walkable_grounded || target_reached || timeout)
			{
				_climbing = false;
				// Suppress ground snap briefly: when we lifted to target_h
				// the capsule is above the step but not yet over it
				// horizontally — snap would yank it back down to the
				// surface in front of the step before the player has time
				// to walk forward onto the step's top.
				_post_climb_flag.stamp();
			} else
			{
				// Force grounded for downstream consumers (state machine,
				// animation, jump coyote) — the lift is the player's "ground".
				_is_grounded = true;
				_walkable_grounded = true;
			}
		}

		// Update descent state. Mirror of climb: exit on walkable contact
		// (landed on the lower surface), reaching the target height, or
		// timeout (e.g. surface vanished under us so the descent target is
		// no longer reachable).
		if (_descending)
		{
			_descent_time += adaptive_time_step;
			Scalar current_h = dot(_world_transform.getTranslate(), Vec3(_up));
			bool target_reached = current_h <= Scalar(_descent_target_height) + Scalar(0.005);
			bool timeout = _descent_time > 1.0f;
			if (_walkable_grounded || target_reached || timeout)
			{
				_descending = false;
			} else
			{
				// Force grounded — the descent IS the player's ground motion.
				_is_grounded = true;
				_walkable_grounded = true;
			}
		}

		if (!_climbing && _hit_wall && _is_grounded && stepHeight > 0.0f && length2(horiz_step) > Consts::EPS)
			try_auto_step(pre_motion_transform, horiz_step, adaptive_time_step);

		rotate(_ctx.rotate_target, _ctx.turn_responsiveness, adaptive_time_step);
	}

	// Walkable wins over slidable: if we touch a walkable surface anywhere,
	// the character is treated as standing on it, not sliding.
	// (`_walkable_flag` is stamped inside the substep loop above — both on
	// walkable contacts and on wall contacts, so brushing stair corner edges
	// whose normals fall into the slidable range doesn't chatter into SLIDE.)
	// Hysteresis: if we just had a walkable contact, suppress slide entry.
	// Otherwise grazing a steep slope while walking on flat would chatter
	// SLIDE↔MOVE (capsule's lower hemisphere intermittently loses the flat
	// contact while still touching the slope).
	bool slide_suppressed = _walkable_flag.isFresh(slideEntryDelay);
	_on_steep_slope = !_walkable_grounded && (_steep_slope_normal != vec3_zero) && !slide_suppressed;
	if (_on_steep_slope)
		_steep_slope_normal = normalizeValid(_steep_slope_normal);
	else
		_steep_slope_normal = _up;

	// Ground snap on descent: when stepping off a small ledge or running
	// down stairs, the capsule briefly free-falls between the surface it
	// just left and the one below. Without it that's several frames of
	// airborne state and a visible fall animation. Instead of teleporting
	// the capsule down (instant jolt and visible "skipping" between steps),
	// queue a gradual descent — substep loop pulls capsule down at
	// stepClimbSpeed while keeping it grounded.
	//
	// Gated by:
	// - !_is_grounded: only when we lost contact this frame.
	// - _vertical_speed <= 0: only when falling, never during a jump.
	// - _grounded_flag.isFresh(coyoteTime): only if we were grounded a moment
	//   ago — intentional jumps off cliffs aren't snapped down.
	// - !_post_climb_flag.isFresh: don't tug back down right after climbing.
	// - !_climbing && !_descending: don't fight ongoing vertical adjustments.
	if (!_is_grounded && _vertical_speed <= 0.0f && _grounded_flag.isFresh(coyoteTime) && !_post_climb_flag.isFresh(0.3f) && !_climbing && !_descending)
	{
		_body->setTransform(_world_transform);
		Vec3 origin = _shape->getBottomCap();
		Scalar radius = Scalar(_shape->getRadius());
		Scalar ray_len = radius + Scalar(stepHeight);
		Vec3 ray_end = origin + Vec3(_gravity_direction) * ray_len;

		WorldIntersectionNormalPtr hit = WorldIntersectionNormal::create();
		auto obj = World::getIntersection(origin, ray_end, groundCheckIntersectionMask, {body}, hit);
		if (obj && dot(hit->getNormal(), _up) > _slope_cos)
		{
			Scalar gap = length(hit->getPoint() - origin) - radius;
			if (gap > Scalar(groundSnapMinGap))
			{
				Scalar current_h = dot(_world_transform.getTranslate(), Vec3(_up));
				_descending = true;
				_descent_target_height = toFloat(current_h - gap);
				_descent_time = 0.0f;
				_vertical_speed = 0.0f;
			}
		}
	}

	target->setWorldTransform(_world_transform);
	body->setWorldTransform(target->getWorldTransform());

	_horizontal_velocity = controlled_horizontal_velocity;
	if (_damage_knockback_timer > 0.0f)
	{
		_damage_knockback_timer = max(_damage_knockback_timer - ifps, 0.0f);
		if (compare(_damage_knockback_timer, 0.0f))
		{
			_damage_knockback_velocity = Vec3_zero;
			_damage_knockback_duration = 0.0f;
		}
	}

	//%%%%%%%%%%%%%%%%%%% Anim %%%%%%%%%%%%%%%
	{
		bool is_sliding = _current_state == MovementStateIndex::SLIDE;
		bool is_moving = !is_sliding && !compare(_ctx.speed, 0.0f);
		bool is_spinting = !is_sliding && abs(_ctx.speed) > runSpeed;
		// Smooth out micro losses of ground contact at surface seams: while
		// the player is descending/standing and was on ground a moment ago,
		// keep the animation in "grounded" state. Cleared on jump (the flag
		// is reset there), so falling/jumping still shows airborne anim.
		bool is_grounded = _is_grounded || (_vertical_speed <= 0.0f && _grounded_flag.isFresh(groundedAnimCoyote));
		bool is_idle = !is_moving && !is_spinting && !is_sliding && is_grounded;

		SET_ANIM_BOOL_PARAM(_anim, "is_moving", is_moving);
		SET_ANIM_BOOL_PARAM(_anim, "is_sprinting", is_spinting);
		SET_ANIM_BOOL_PARAM(_anim, "is_grounded", is_grounded);
		SET_ANIM_BOOL_PARAM(_anim, "is_idle", is_idle);
		SET_ANIM_BOOL_PARAM(_anim, "is_sliding", is_sliding);
		_anim->setParamFloat("rand_float", Game::getRandomFloat(0.0f, 1.0f));
	}

	//%%%%%%%%%%%%%%%%%%% Shape %%%%%%%%%%%%%%%
	{
		// Use the same stabilized "grounded" signal as the animation block:
		// raw _is_grounded flickers on flat surfaces (FP-noise contact loss),
		// and shrinking the capsule lifts its bottom — once shrunk it's even
		// less likely to contact ground next frame, producing a fall/run/fall
		// loop. Coyote-grace prevents that.
		bool stable_grounded = _is_grounded || (_vertical_speed <= 0.0f && _grounded_flag.isFresh(groundedAnimCoyote));
		_shape->setHeight(_shape_height * (stable_grounded ? 1.0f : fall_scale));
	}

	//%%%%%%%%%%%%%%%%%%% Debug %%%%%%%%%%%%%%%
	{
		// Log::message("CharacterMovement State: %s\n", _states[_current_state]->name());
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

void CharacterMovement::applyDamageKnockback(const Vec3 &source_position)
{
	const float horizontal_speed = max(damageKnockbackSpeed.get(), 0.0f);
	const float duration = max(damageKnockbackDuration.get(), 0.0f);

	if (horizontal_speed <= 0.0f || duration <= 0.0f)
	{
		return;
	}

	const Vec3 self_position = target ? target->getWorldPosition() : _world_transform.getTranslate();
	vec3 direction = vec3(self_position - source_position);
	direction -= _up * dot(direction, _up);
	if (length2(direction) <= Consts::EPS)
	{
		direction = -vec3(_world_transform.getAxisY());
		direction -= _up * dot(direction, _up);
	}

	direction = normalizeValid(direction);
	if (direction == vec3_zero)
	{
		return;
	}

	_damage_knockback_velocity = Vec3(direction) * Scalar(horizontal_speed);
	_damage_knockback_timer = duration;
	_damage_knockback_duration = duration;
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

		if (DebugTuning::get()->show_movement_rays)
		{
			Visualizer::renderVector(p0, p0 + down_ray, vec4_blue, 0.01f);
		}

		if (object)
		{
			if (DebugTuning::get()->show_movement_hit)
			{
				Visualizer::renderPoint3D(hit_normal->getPoint(), 0.01f, vec4_blue, false, 0.0f, false);
			}

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

			// Push dynamic rigid bodies (e.g. dead-skull corpses): the character
			// is kinematic, so contacts don't naturally transfer momentum.
			// Apply only on iter==0 — _shape->getCollision is a query, the
			// rigid body doesn't move between iterations, so the same contact
			// would otherwise compound the impulse N times per substep.
			if (iter == 0 && pushStrength > 0.0f)
			{
				ShapePtr s0 = contact->getShape0();
				ShapePtr s1 = contact->getShape1();
				ShapePtr other_shape = (s0.get() == _shape.get()) ? s1 : s0;
				if (other_shape)
				{
					BodyRigidPtr other_body = checked_ptr_cast<BodyRigid>(other_shape->getBody());
					// Skip alive Entities: they control their own velocity (AI
					// steering overrides our impulse anyway), and for kamikaze
					// enemies our predictive push throws them away before the
					// physics ContactEnter event fires — damage never lands.
					// Dead corpses, static rocks, props all pass this check.
					bool other_is_alive_entity = false;
					if (other_body)
					{
						if (auto other_obj = other_body->getObject())
						{
							auto entity = ComponentSystem::get()->getComponent<Entity>(other_obj);
							if (entity && entity->isAlive())
								other_is_alive_entity = true;
						}
					}
					if (other_body && other_body->getMass() > 0.0f && !other_is_alive_entity)
					{
						vec3 push_dir = -normal;
						vec3 char_v = vec3(_horizontal_velocity) + _up * _vertical_speed;
						vec3 r = vec3(contact_point - other_body->getWorldCenterOfMass());
						vec3 body_v = other_body->getLinearVelocity() + cross(other_body->getAngularVelocity(), r);
						float delta = dot(char_v - body_v, push_dir);
						if (delta > 0.0f)
						{
							// Cap how fast we can drive the body through this contact —
							// without it, light corpses with low damping shoot off at
							// absurd speeds when sprinted into.
							float cap = pushMaxSpeed;
							if (cap > 0.0f)
							{
								float body_speed_along = dot(body_v, push_dir);
								delta = min(delta, max(cap - body_speed_along, 0.0f));
							}
							if (delta > 0.0f)
							{
								if (other_body->isFrozen())
									other_body->setFrozen(false);
								vec3 impulse = push_dir * (other_body->getMass() * delta * pushStrength);
								other_body->addWorldImpulse(contact_point, impulse);
							}
						}
					}
				}
			}

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

			if (DebugTuning::get()->show_movement_contact_points)
			{
				Visualizer::renderPoint3D(contact_point, 0.01f, vec4_red);
				Visualizer::renderVector(contact_point, contact_point + Vec3(normal), vec4_black);
			}

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
				} else
				{
					// Slidable. Distinguish a real slope (motion glides along
					// the surface, normal_speed ≈ 0) from a stair-corner edge
					// or any obstacle the player is walking INTO (normal_speed
					// < 0). Only the former should accumulate as a slide
					// candidate — otherwise SlideState hijacks stair-climbing
					// and pushes the player away from obstacles.
					if (normal_speed >= 0.0f)
					{
						_steep_slope_normal += normal;
					} else
					{
						_hit_wall = true;
						_hit_wall_normal += normal;
					}
				}
			} else if (normal_speed < 0.0f)
			{
				// Side/wall contact pushing back against horizontal motion —
				// candidate for auto-stepping over a low obstacle.
				_hit_wall = true;
				_hit_wall_normal += normal;
			}
		}

		_world_transform.setColumn3(3, _world_transform.getTranslate() + Vec3(pos_offset));
	}
}

void CharacterMovement::try_auto_step(const Mat4 &pre_motion, const Vec3 &horiz_motion, float ifps)
{
	Mat4 post_resolved = _world_transform;

	// Direction to push the probe past the obstacle. Prefer the wall's outward
	// normal (so the capsule clears perpendicular to the wall regardless of
	// which way the player is heading — needed to climb when running diagonally
	// into stairs). Fall back to motion direction if no wall normal recorded.
	Vec3 motion_horiz = horiz_motion - Vec3(_up) * Scalar(toFloat(dot(horiz_motion, Vec3(_up))));
	Scalar motion_dist = length(motion_horiz);

	Vec3 step_dir = Vec3_zero;
	if (length2(_hit_wall_normal) > Consts::EPS)
	{
		vec3 wall_n = normalizeValid(_hit_wall_normal);
		vec3 dir = -wall_n;
		dir -= _up * dot(dir, _up);
		if (length2(dir) > Consts::EPS)
			step_dir = Vec3(normalizeValid(dir));
	}
	if (length2(step_dir) < Consts::EPS)
		step_dir = Vec3(normalizeValid(vec3(motion_horiz)));

	// Skip auto-step if the player is moving nearly tangent to the wall.
	// Otherwise capsule bumps the wall corner with a tiny perpendicular
	// component, climb engages, but lifting + 1s timeout completes before
	// the slow horizontal approach actually reaches the step's top — capsule
	// falls back, hits the wall again, climb engages again. Cycle.
	// Threshold: cos(angle from perpendicular) >= 0.2 (~78° max approach).
	Scalar motion_perp = dot(motion_horiz, step_dir);
	if (motion_perp < motion_dist * Scalar(0.2))
		return;

	// Probe at least far enough that the capsule's center clears the wall
	// (one radius), but never less than the substep's intended motion. This
	// is the perpendicular displacement to wall — the player's actual motion
	// (which may be diagonal) doesn't matter for the probe.
	Scalar min_forward = Scalar(_shape->getRadius() + 0.02);
	Scalar forward_dist = motion_dist > min_forward ? motion_dist : min_forward;

	// Probe position: pre-motion lifted by stepHeight, then translated past
	// the wall edge along step_dir.
	Mat4 elevated = pre_motion;
	elevated.setColumn3(3, pre_motion.getTranslate() + Vec3(_up) * Scalar(stepHeight) + step_dir * forward_dist);

	_body->setTransform(elevated);
	_shape->getCollision(_contacts, ifps);

	Vec3 elevated_bottom_cap = _shape->getBottomCap();

	// If anything still pushes back against motion at the elevated probe, the
	// obstacle is too tall to step over.
	int probe_count = Math::min(_contacts.size(), 16);
	for (int i = 0; i < probe_count; ++i)
	{
		const ConstShapeContactPtr &c = _contacts[i];
		vec3 normal = c->getNormal();
		Vec3 contact_point = c->getPoint();
		float slope_dot = dot(normal, _up);
		bool is_below = dot(contact_point - elevated_bottom_cap, Vec3(_up)) < 0.0f;
		if (is_below && slope_dot > _slope_cos)
			continue; // walkable below — that's the step we'd land on
		// Ceiling-like contact at the elevated probe (normal points down)
		// means lifting into geometry above. The step is too tall to fit
		// under whatever is overhead — abort instead of teleporting forward.
		if (slope_dot < 0.0f)
			return;
		float v_into = toFloat(dot(Vec3(normal), _horizontal_velocity));
		if (v_into < 0.0f)
			return;
	}

	// Drop down from the elevated probe to find the surface to land on.
	Scalar radius = Scalar(_shape->getRadius());
	Vec3 ray_origin = elevated_bottom_cap;
	Vec3 ray_end = ray_origin + Vec3(_gravity_direction) * (radius + Scalar(stepHeight) + Scalar(0.05));

	WorldIntersectionNormalPtr hit = WorldIntersectionNormal::create();
	auto obj = World::getIntersection(ray_origin, ray_end, groundCheckIntersectionMask, {body}, hit);
	if (!obj || dot(hit->getNormal(), _up) < _slope_cos)
		return;

	// drop > 0: ray hit is below capsule's bottom — descend to land.
	// drop < 0: ray hit is above capsule's bottom — the obstacle's top
	//           sticks above where stepHeight lifted us, so we'd actually
	//           need to lift further. Don't clamp — let height_gain reflect
	//           the true step height so the stepHeight check rejects it.
	Scalar drop = length(hit->getPoint() - ray_origin) - radius;

	Vec3 final_pos = elevated.getTranslate() + Vec3(_gravity_direction) * drop;

	// Only commit if we're actually stepping UP (and not by more than the
	// configured max). A non-positive height gain means the probe found the
	// same surface or below — let regular physics handle that.
	Scalar height_gain = dot(final_pos - post_resolved.getTranslate(), Vec3(_up));
	if (height_gain <= Scalar(0.0) || height_gain > Scalar(stepHeight))
		return;

	// Don't teleport. Queue a vertical climb: the substep loop lifts the
	// capsule gradually toward this absolute target height while suppressing
	// gravity and forcing grounded state. The player walks forward naturally
	// once the lift carries them above the obstacle.
	_climbing = true;
	_climb_target_height = toFloat(dot(final_pos, Vec3(_up)));
	_climb_time = 0.0f;
}

void CharacterMovement::rotate(const vec3 &direction, float turn_responsiveness, float ifps)
{
	// if character's up and _up are parallel, then it should be ok to skip normalize().
	// we assume they are parallel
	vec3 forward = _world_transform.getRotate() * vec3_forward;
	// forward = normalize(forward - _up * dot(forward, _up));

	float angle = Math::atan2(
					  dot(cross(forward, direction), _up),
					  dot(forward, direction)) *
				  Consts::RAD2DEG;

	// Exponential damping toward target: fast initial rotation that eases into
	// the cel. turn_responsiveness is a rate (1/s) — at rate R, ~63% of the
	// remaining angle is covered in 1/R seconds, ~95% in 3/R. Huge values
	// approach an instant snap (used for from-standstill alignment in MoveState).
	float smooth = 1.0f - Math::exp(-turn_responsiveness * ifps);
	float step = angle * smooth;

	quat delta_rot = quat(_up, step);
	quat current_rot = _world_transform.getRotate();

	_world_transform = Mat4(delta_rot * current_rot, _world_transform.getTranslate());
}
