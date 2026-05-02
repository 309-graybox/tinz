#include "MoveState.h"
#include "IdleState.h"
#include "CharacterMovement.h"

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
	ctx.turn_speed = ctx.input.isSprinting() ? o.sprintTurnSpeed
											 : o.turnSpeed;

	ctx.move_direction = o.project_forward_on_ground(ctx.ground_normal);
	ctx.rotate_target = ctx.desired_input_direction;
	return MovementStateIndex::NONE;
}