#pragma once
#include "MovementState.h"

class MoveState : public MovementState
{
public:
	void init();
	MovementStateIndex update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Move"; }
};