#include "MoveState.h"
#include "IdleState.h"
#include "TurnState.h"
#include "CharacterMovement.h"

using namespace Unigine;
using namespace Math;

MoveState::MoveState(bool snape_rotate)
	: _snap_rotation(snape_rotate)
{
}

MovementState *MoveState::update(MovementContext &ctx, float ifps)
{
	auto &o = *ctx.owner;

	if (!ctx.input.isInputMoving())
	{
		return new IdleState();
	}

	// jump just for now
	ctx.vertical_impulse = 0.0f;
	if (ctx.input.consumeJump() && ctx.is_grounded)
	{
		ctx.vertical_impulse = o.jumpPower / ifps;
		// crouch just for now
	} else if (ctx.input.isCrouching() && !ctx.is_grounded)
	{
		ctx.vertical_impulse = -o.jumpPower / ifps;
	}

	ctx.speed = ctx.input.isSprinting() ? o.sprintSpeed
										: ctx.input.isWalking() ? o.walkSpeed
																: o.runSpeed;
	ctx.turn_speed = ctx.input.isSprinting() ? o.sprintTurnSpeed
											 : o.turnSpeed;

	ctx.move_direction = o.project_forward_on_ground(ctx.ground_normal);

	// cos: +1 -> 0 -> -1
	float cos_move_direction = dot(ctx.desired_input_direction, ctx.character_forward);

	if (_snap_rotation)
	{
		_snap_rotation = false;
		ctx.turn_speed = 1e6f;
		ctx.rotate_target = ctx.desired_input_direction;
		return nullptr;
	}

	if (ctx.input.isSprinting() && cos_move_direction < o._sharp_turn_cos)
	{
		return new TurnState(ctx.desired_input_direction, o.sprintSharpTurnSpeed);
	}

	ctx.rotate_target = ctx.desired_input_direction;
	return nullptr;
}