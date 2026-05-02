#include "IdleState.h"
#include "MoveState.h"
#include "CharacterMovement.h"
#include "MovementContext.h"
#include "MovementState.h"

void IdleState::init()
{
}

MovementStateIndex IdleState::update(MovementContext &ctx, float ifps)
{
	if (ctx.is_on_steep_slope)
	{
		ctx.owner->_slide_state.init();
		return MovementStateIndex::SLIDE;
	}

	if (ctx.input.isInputMoving())
	{
		ctx.owner->_move_state.init();
		return MovementStateIndex::MOVE;
	}

	ctx.speed = 0.0f;
	ctx.rotate_target = ctx.character_forward;

	return MovementStateIndex::NONE;
}