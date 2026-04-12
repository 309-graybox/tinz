#include "IdleState.h"
#include "MoveState.h"
#include "MovementContext.h"

MovementState *IdleState::update(MovementContext &ctx, float ifps)
{
	if (ctx.input.isInputMoving())
	{
		return new MoveState();
	}

	ctx.speed = 0.0f;
	ctx.rotate_target = ctx.character_forward;

	return nullptr;
}