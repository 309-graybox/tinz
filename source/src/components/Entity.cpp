#include "Entity.h"

#include <UnigineGame.h>

REGISTER_COMPONENT(Entity)

using namespace Unigine;
using namespace Unigine::Math;

bool Entity::isInvulnerable() const noexcept
{
	return Game::getTime() < _invulnerable_until;
}

bool Entity::takeDamage(const DamageInfo &damageInfo)
{
	const float amount = damageInfo.amount;
	const bool receivesDamage = amount > 0.0f;
	if (isDead())
	{
		Log::message("%s damage ignored: already dead, amount: %.2f\n", node->getName(), amount);
		return false;
	}

	if (receivesDamage && isInvulnerable())
	{
		Log::message("%s damage ignored: invulnerable, amount: %.2f, time left: %.2f\n",
			node->getName(), amount, max(_invulnerable_until - Game::getTime(), 0.0f));
		return false;
	}

	const float old_hp = _hp;
	_hp = max(0.0f, _hp - amount);
	if (receivesDamage)
		_invulnerable_until = Game::getTime() + max(invulnerabilityTime.get(), 0.0f);

	Log::message("%s hp changed: %.2f -> %.2f, amount: %.2f, invulnerability: %.2f\n",
		node->getName(), old_hp, _hp, amount, receivesDamage ? max(invulnerabilityTime.get(), 0.0f) : 0.0f);

	if (isDead())
	{
		Log::message("%s died\n", node->getName());
		_event_died.run(this);
		if (!persistOnDeath)
			node.deleteLater();
	}

	return !compare(old_hp, _hp);
}

void Entity::revive()
{
	_hp = max_hp;
	_invulnerable_until = 0.0f;
}

void Entity::init()
{
	_hp = max_hp;
	_invulnerable_until = 0.0f;
}

void Entity::update()
{
}

void Entity::shutdown()
{
}
