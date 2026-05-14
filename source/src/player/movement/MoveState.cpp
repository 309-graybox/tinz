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

	// Move direction follows INPUT, not facing. Velocity is always along where
	// the player is pressing (projected on the ground plane). Body facing
	// catches up via the damped rotate() in CharacterMovement.
	//
	// Why not facing: with velocity = facing × speed, rapid L/R/L/R taps make
	// the body oscillate-pivot through camera-forward; the forward component
	// of facing during each pivot accumulates into a slow forward creep while
	// the L/R components cancel. Anchoring move_direction to input fixes that
	// directly — alternating taps produce alternating velocity vectors that
	// cancel cleanly. Visible "body lagging behind motion" sliding is kept
	// small by the speed multiplier below: when facing is far from input,
	// speed is scaled down so the offset isn't doing much movement.
	vec3 input_on_ground = ctx.desired_input_direction;
	float up_normal_cos = dot(o._up, ctx.ground_normal);
	if (Math::abs(up_normal_cos) >= Consts::EPS)
		input_on_ground -= o._up * (dot(input_on_ground, ctx.ground_normal) / up_normal_cos);
	input_on_ground = normalizeValid(input_on_ground);
	vec3 fallback_forward = o.project_forward_on_ground(ctx.ground_normal);
	ctx.move_direction = (input_on_ground != vec3_zero) ? input_on_ground : fallback_forward;

	// Starting from a standstill: snap rotation toward the input direction so
	// the character doesn't visibly spin up to facing before accelerating.
	// Once moving, fall back to the gradual damped rotation.
	//
	// "Standstill" = current velocity is ~zero AND we haven't been running in
	// the last ~recent_move_window seconds. The second clause prevents the
	// snap from firing on a 180° turn, where the player briefly passes
	// through IDLE between releasing one key and pressing the opposite one
	// — without it, that gap zeroes the velocity and the snap would hijack
	// what should be a gradual damped rotation.
	constexpr float recent_move_window = 0.2f;
	bool from_standstill = length2(o._horizontal_velocity) < Consts::EPS
						   && !_moving_flag.isFresh(recent_move_window);
	if (from_standstill && length2(ctx.desired_input_direction) > Consts::EPS)
	{
		// Huge rate so exp-damped rotate() consumes any angle in one substep —
		// effectively instant snap to input direction from idle.
		ctx.turn_responsiveness = 1.0e6f;
	} else
	{
		ctx.turn_responsiveness = ctx.input.isSprinting() ? o.sprintTurnResponsiveness
														  : o.turnResponsiveness;

		// Continuous facing-vs-input speed multiplier. The bigger the angle
		// between current facing and input, the slower we move — full speed
		// below fullSpeedAngle, zero at plantAngle, linear blend between.
		// Combined with damped rotation this produces:
		//   - small course corrections: barely any slowdown, smooth arc
		//   - sharp turns: visible decel into the turn, accel back out
		//   - ~180° flips: speed planted at 0 while character pivots, then accel
		// Gated on ground: airborne already preserves momentum and steers via
		// airControl — applying this here would silently kill air control.
		if (ctx.is_grounded)
		{
			float align_cos = dot(ctx.character_forward, ctx.desired_input_direction);
			float full_cos  = Math::cos(o.fullSpeedAngle * Consts::DEG2RAD);
			float plant_cos = Math::cos(o.plantAngle    * Consts::DEG2RAD);
			float denom = Math::max(full_cos - plant_cos, Consts::EPS);
			float k = saturate((align_cos - plant_cos) / denom);
			ctx.speed *= k;
		}
	}

	ctx.rotate_target = ctx.desired_input_direction;
	_moving_flag.stamp();
	return MovementStateIndex::NONE;
}