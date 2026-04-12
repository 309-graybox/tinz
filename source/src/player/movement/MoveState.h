#pragma once
#include "MovementState.h"

class MoveState : public MovementState
{
public:
	MoveState(bool snap_rotation = false);
	MovementState *update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Move"; }

private:
	bool _snap_rotation = false;
};