#pragma once
#include "PlayerState.h"

class PlayerStateWalk: public PlayerState
{
public:
	PlayerStateWalk(PlayerContext &ctx, const char *actionName);

	const char *getStateName() const override { return "Walk"; }
	float getScore() const override;

private:
	Unigine::Math::vec3 _move;
};
