#include "Entity.h"

REGISTER_COMPONENT(Entity)

using namespace Unigine;
using namespace Unigine::Math;

void Entity::takeDamage(const DamageInfo &damageInfo)
{
	if (isDead())
		return;

	_hp = max(0.0f, _hp - damageInfo.amount);

	Log::message("%s new hp: %f\n", node->getName(), _hp);

	if (isDead())
	{
		_event_died.run(this);
		if (!persistOnDeath)
			node.deleteLater();
	}
}

void Entity::revive()
{
	_hp = max_hp;
}

void Entity::init()
{
	_hp = max_hp;
}

void Entity::update()
{
}

void Entity::shutdown()
{
}
