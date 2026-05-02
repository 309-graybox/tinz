#include "SlideState.h"
#include "MoveState.h"
#include "IdleState.h"
#include "CharacterMovement.h"
#include "MovementContext.h"

#include <UnigineMathLibCommon.h>

using namespace Unigine;
using namespace Math;

void SlideState::init()
{
	_slide_speed = 0.0f;
	_slide_base = vec3_zero;
}

void SlideState::onEnter(MovementContext &ctx)
{
	auto &o = *ctx.owner;
	vec3 down = -o._up;
	_slide_base = normalizeValid(down - ctx.steep_slope_normal * dot(down, ctx.steep_slope_normal));

	// Carry over existing momentum so hitting a slope at speed (running, falling)
	// keeps moving instead of snapping to zero.
	vec3 prev_velocity = vec3(o._horizontal_velocity) + o._up * o._vertical_speed;
	float carry = dot(prev_velocity, _slide_base);
	_slide_speed = max(0.0f, carry);
}

MovementStateIndex SlideState::update(MovementContext &ctx, float ifps)
{
	auto &o = *ctx.owner;

	// Exit: in air, or surface flat enough to walk (escape hysteresis).
	bool escape_walkable = ctx.max_below_slope_dot > o._escape_slope_cos;
	if (!ctx.is_grounded || escape_walkable)
	{
		if (ctx.input.isInputMoving())
		{
			o._move_state.init();
			return MovementStateIndex::MOVE;
		}
		o._idle_state.init();
		return MovementStateIndex::IDLE;
	}

	// Pick the freshest slope normal we have — when we're in the 40-43°
	// hysteresis zone the contact is classified walkable, so steep_slope_normal
	// is unavailable. Reuse the cached base in that case.
	vec3 slope_normal = ctx.is_on_steep_slope ? ctx.steep_slope_normal : -o._gravity_direction;
	vec3 down = -o._up;
	if (ctx.is_on_steep_slope)
		_slide_base = normalizeValid(down - slope_normal * dot(down, slope_normal));

	if (_slide_base == vec3_zero)
		_slide_base = ctx.character_forward;

	// Lateral axis in the slope plane, pointing to the player's right relative
	// to the downhill direction.
	vec3 slide_lateral = normalizeValid(cross(_slide_base, slope_normal));

	// Project player's world-space input onto the slope plane and decompose
	// into forward (along slide_base) and lateral components.
	vec3 input_w = ctx.desired_input_direction;
	vec3 input_in_plane = input_w - slope_normal * dot(input_w, slope_normal);
	float forward_input = dot(input_in_plane, _slide_base);
	float lateral_input = dot(input_in_plane, slide_lateral);

	// Speed multiplier from input: forward accelerates, back slows.
	float input_mult = 1.0f;
	if (forward_input >= 0.0f)
		input_mult = lerp(1.0f, o.slideForwardMultiplier, saturate(forward_input));
	else
		input_mult = lerp(1.0f, o.slideBackMultiplier, saturate(-forward_input));

	// Slope multiplier interpolated by current angle from slopeLimit (min) to
	// slideMaxAngle (max).
	float slope_dot = saturate(dot(slope_normal, o._up));
	float angle_deg = Math::acos(slope_dot) * Consts::RAD2DEG;
	float t_slope = saturate((angle_deg - o.slopeLimit) / max(o.slideMaxAngle - o.slopeLimit, Consts::EPS));
	float slope_mult = lerp(o.slideMinSlopeMultiplier, o.slideMaxSlopeMultiplier, t_slope);

	float target_speed = o.baseSlideSpeed * input_mult * slope_mult;

	// Smooth ramp toward target.
	float delta = target_speed - _slide_speed;
	float max_change = o.slideAcceleration * ifps;
	_slide_speed += clamp(delta, -max_change, max_change);

	// Apply lateral steering on top of the base slide direction.
	vec3 move_dir = normalizeValid(_slide_base + slide_lateral * lateral_input * o.slideLateralStrength);
	if (move_dir == vec3_zero)
		move_dir = _slide_base;

	ctx.move_direction = move_dir;
	ctx.speed = _slide_speed;
	ctx.rotate_target = move_dir;
	ctx.turn_speed = o.turnSpeed;

	return MovementStateIndex::NONE;
}
