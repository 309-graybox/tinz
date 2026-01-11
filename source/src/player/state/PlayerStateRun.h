#pragma once
#include "PlayerState.h"

class PlayerStateRun: public PlayerState
{
public:
	PlayerStateRun(PlayerContext &ctx, const char *actionName);

	const char *getStateName() const override { return "Run"; }
	float getScore() const override;

private:
	bool _sprint;
};
