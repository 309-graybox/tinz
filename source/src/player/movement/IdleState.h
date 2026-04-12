#pragma once
#include "MovementState.h"

class IdleState : public MovementState
{
public:
	MovementState *update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Idle"; }
};