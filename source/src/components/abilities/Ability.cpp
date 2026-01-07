#include "Ability.h"
#include <UnigineGame.h>
#include <UnigineMathLib.h>

REGISTER_COMPONENT(Ability)

using namespace Unigine;
using namespace Unigine::Math;

bool Ability::isInCooldown() const
{
	return !Unigine::Math::compare(currentCooldown, 0.0f);
}

void Ability::init()
{
	currentCooldown = 0.0f;
}

void Ability::update()
{
	currentCooldown = max(currentCooldown - Game::getIFps(), 0.0f);
}

void Ability::use()
{
	if (canBeUsed())
	{
		Log::message("Use %s\n", getClassName());
		useImpl();
		currentCooldown = cooldown.get(); // TODO WTF is this? why currentCooldown = cooldown makes ref copy???
	}
}
