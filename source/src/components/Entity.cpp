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
	const bool receivesDamage = damageInfo.amount > 0.0f;
	if (isDead() || (receivesDamage && isInvulnerable()))
		return false;

	const float old_hp = _hp;
	_hp = max(0.0f, _hp - damageInfo.amount);
	if (receivesDamage)
		_invulnerable_until = Game::getTime() + max(invulnerabilityTime.get(), 0.0f);

	Log::message("%s new hp: %f\n", node->getName(), _hp);

	if (isDead())
	{
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
