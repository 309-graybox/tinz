#include "Lifetime.h"
#include <UnigineGame.h>

REGISTER_COMPONENT(Lifetime)

using namespace Unigine;
using namespace Unigine::Math;

void Lifetime::init()
{
	if (use_time)
		_remainingTime = time;
	else
		_remainingFrames = frames;
}

void Lifetime::update()
{
	if (use_time)
	{
		_remainingTime -= Game::getIFps();
		if (_remainingTime <= 0)
			node.deleteLater();
	} else
	{
		--_remainingFrames;
		if (_remainingFrames <= 0)
			node.deleteLater();
	}
}
