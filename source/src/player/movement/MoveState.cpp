#include "MoveState.h"
#include "IdleState.h"
#include "CharacterMovement.h"

#include <UnigineMathLibCommon.h>

using namespace Unigine;
using namespace Math;

void MoveState::init()
{
}

MovementStateIndex MoveState::update(MovementContext &ctx, float ifps)
{
	auto &o = *ctx.owner;

	if (ctx.is_on_steep_slope)
	{
		o._slide_state.init();
		return MovementStateIndex::SLIDE;
	}

	if (!ctx.input.isInputMoving())
	{
		o._idle_state.init();
		return MovementStateIndex::IDLE;
	}

	// crouch just for now
	if (ctx.input.isCrouching() && !ctx.is_grounded)
		ctx.vertical_impulse = -o.jumpPower / ifps;

	ctx.speed = ctx.input.isSprinting() ? o.sprintSpeed
										: ctx.input.isWalking() ? o.walkSpeed
																: o.runSpeed;

	// Starting from a standstill: snap rotation toward the input direction and
	// move in that direction immediately, so the character doesn't visibly
	// spin up to facing before accelerating. Once moving, fall back to the
	// gradual turnSpeed rotation around the current facing.
	//
	// "Standstill" = current velocity is ~zero AND we haven't been running in
	// the last ~recent_move_window seconds. The second clause prevents the
	// snap from firing on a 180° turn, where the player briefly passes
	// through IDLE between releasing one key and pressing the opposite one
	// — without it, that gap zeroes the velocity and the snap would hijack
	// what should be a gradual turnSpeed rotation.
	constexpr float recent_move_window = 0.2f;
	bool from_standstill = length2(o._horizontal_velocity) < Consts::EPS
						   && !_moving_flag.isFresh(recent_move_window);
	if (from_standstill && length2(ctx.desired_input_direction) > Consts::EPS)
	{
		vec3 input_dir = ctx.desired_input_direction;
		float up_normal_cos = dot(o._up, ctx.ground_normal);
		if (Math::abs(up_normal_cos) >= Consts::EPS)
			input_dir -= o._up * (dot(input_dir, ctx.ground_normal) / up_normal_cos);
		input_dir = normalizeValid(input_dir);
		ctx.move_direction = (input_dir != vec3_zero)
								 ? input_dir
								 : o.project_forward_on_ground(ctx.ground_normal);
		// Huge value so rotate() consumes any angle in a single substep
		// regardless of ifps — effectively instant.
		ctx.turn_speed = 1.0e6f;
	} else
	{
		ctx.move_direction = o.project_forward_on_ground(ctx.ground_normal);
		ctx.turn_speed = ctx.input.isSprinting() ? o.sprintTurnSpeed
												 : o.turnSpeed;
	}

	ctx.rotate_target = ctx.desired_input_direction;
	_moving_flag.stamp();
	return MovementStateIndex::NONE;
}