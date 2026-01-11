#pragma once
#include "PlayerState.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class PlayerStateCrouchIdle: public PlayerState
{
public:
	PlayerStateCrouchIdle(PlayerContext &ctx, const char *actionName);

	const char *getStateName() const override { return "Crouch"; }
	float getScore() const override;

private:
	bool _crouch;
};
