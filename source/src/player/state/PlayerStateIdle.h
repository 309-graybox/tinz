#pragma once
#include "PlayerState.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class PlayerStateIdle: public PlayerState
{
public:
	PlayerStateIdle(PlayerContext &ctx);

	const char *getStateName() const override { return "Idle"; }
	float getScore() const override;
};
