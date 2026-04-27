#include "MoveState.h"
#include "IdleState.h"
#include "TurnState.h"
#include "CharacterMovement.h"

using namespace Unigine;
using namespace Math;

void MoveState::init(bool snap_rotation)
{
	_snap_rotation = snap_rotation;
}

MovementStateIndex MoveState::update(MovementContext &ctx, float ifps)
{
	auto &o = *ctx.owner;

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
		return MovementStateIndex::NONE;
	}

	if (ctx.input.isSprinting() && cos_move_direction < o._sharp_turn_cos)
	{
		o._turn_state.init(ctx.desired_input_direction, o.sprintTurnSpeed);
		return MovementStateIndex::TURN;
	}

	ctx.rotate_target = ctx.desired_input_direction;
	return MovementStateIndex::NONE;
}