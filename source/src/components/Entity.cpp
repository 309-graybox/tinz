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
		node.deleteLater();
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
