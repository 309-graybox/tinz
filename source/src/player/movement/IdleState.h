#pragma once
#include "MovementState.h"

class IdleState : public MovementState
{
public:
	MovementState *update(MovementContext &, float ifps) override {
		return nullptr;
	}

	const char *name() const override { return "Idle"; }
};