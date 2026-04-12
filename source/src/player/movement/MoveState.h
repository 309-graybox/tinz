#pragma once
#include "MovementState.h"

class MoveState : public MovementState
{
public:
	MovementState *update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Move"; }
};