#pragma once
#include "TuningBase.h"

class PlayerTuning: public TuningBase<PlayerTuning>
{
public:
	COMPONENT_DEFINE(PlayerTuning, TuningBase<PlayerTuning>)
	COMPONENT_INIT(init, INT_MAX)

private:
	void init();
	void configure();
};
