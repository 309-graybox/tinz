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
	if (ctx.input.isInputMoving())
	{
		ctx.owner->_move_state.init(false);
		return MovementStateIndex::MOVE;
	}

	ctx.speed = 0.0f;
	ctx.rotate_target = ctx.character_forward;

	return MovementStateIndex::NONE;
}