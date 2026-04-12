#pragma once
#include "MovementState.h"
#include <UnigineMathLibVec3.h>

class TurnState : public MovementState
{
public:
	TurnState(const Unigine::Math::vec3 &desired_direction, float turn_speed);
	MovementState *update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Turn"; }

private:
	Unigine::Math::vec3 _lock_direction;
	float _lock_turn_speed;
};