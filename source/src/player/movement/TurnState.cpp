#include "TurnState.h"
#include "MovementContext.h"
#include "CharacterMovement.h"
#include "MoveState.h"

using namespace Unigine;
using namespace Math;

TurnState::TurnState(const vec3 &desired_direction, float turn_speed)
	: _lock_direction(desired_direction),
	_lock_turn_speed(turn_speed)
{
}

MovementState *TurnState::update(MovementContext &ctx, float ifps)
{
	auto &o = *ctx.owner;
	// cos: +1 -> 0 -> -1
	float cos_move_direction = dot(_lock_direction, ctx.character_forward);

	// TODO(vah): remove excessive defaults because we set them before state->update()?
	ctx.speed = 0.0f;
	ctx.turn_speed = _lock_turn_speed;
	ctx.rotate_target = _lock_direction;
	ctx.move_direction = ctx.character_forward;

	return o._turning_exit_cos > cos_move_direction ? nullptr : new MoveState();
}