#pragma once
#include "PlayerState.h"

class PlayerStateIdle: public PlayerState
{
public:
	const char *getStateName() const override { return "Idle"; }

protected:
	void onInitImpl(PlayerContext &ctx) override;
	void onEnterImpl(PlayerContext &ctx) override;
	void onUpdateImpl(PlayerContext &ctx) override;
	void onExitImpl(PlayerContext &ctx) override;
};
