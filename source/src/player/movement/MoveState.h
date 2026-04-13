#pragma once
#include "MovementState.h"

class MoveState : public MovementState
{
public:
	void init(bool snap_rotation = false);
	MovementStateIndex update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Move"; }

private:
	bool _snap_rotation = false;
};