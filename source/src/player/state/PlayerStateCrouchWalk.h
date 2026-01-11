#pragma once
#include "PlayerState.h"

class PlayerStateCrouchWalk: public PlayerState
{
public:
	PlayerStateCrouchWalk(PlayerContext &ctx, const char *actionName);

	const char *getStateName() const override { return "Crouch Walk"; }
	float getScore() const override;

private:
	Unigine::Math::vec3 _move;
};
