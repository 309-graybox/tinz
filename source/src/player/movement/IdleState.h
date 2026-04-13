#pragma once
#include "MovementState.h"

class IdleState : public MovementState
{
public:
	void init();
	MovementStateIndex update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Idle"; }
};