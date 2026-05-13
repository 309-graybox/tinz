#pragma once
#include "MovementState.h"
#include "utils/TimedFlag.h"

class MoveState : public MovementState
{
public:
	void init();
	MovementStateIndex update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Move"; }

private:
	// Stamped each frame this state is active with non-zero speed. Used to
	// distinguish a genuine standstill (stale flag → rotation snap on entry)
	// from a brief release+press while running like a 180° turn (fresh flag
	// → keep gradual turnSpeed).
	TimedFlag _moving_flag;
};