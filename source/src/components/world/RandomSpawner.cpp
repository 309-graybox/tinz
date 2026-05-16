#include "components/world/RandomSpawner.h"

#include "components/Entity.h"

#include <UnigineGame.h>

REGISTER_COMPONENT(RandomSpawner)

using namespace Unigine;
using namespace Unigine::Math;

namespace
{
Entity *findEntityInHierarchy(const NodePtr &root)
{
	return ComponentSystem::get()->getComponentInChildren<Entity>(root);
}
} // namespace

void RandomSpawner::init()
{
	_cooldown_timer = 0.0f;
	_active_spawn.clear();
}

void RandomSpawner::update()
{
	const float dt = Game::getIFps();
	const float cd = max((float)cooldown, 0.0f);

	if (!cooldownAfterDeathOnly)
	{
		if (_cooldown_timer > 0.0f)
			_cooldown_timer = max(_cooldown_timer - dt, 0.0f);

		if (_cooldown_timer <= Consts::EPS && spawnOne())
			_cooldown_timer = cd;
		return;
	}

	if (_active_spawn)
	{
		if (!isActiveSpawnAlive())
		{
			_active_spawn.clear();
			_cooldown_timer = cd;
		}
		return;
	}

	if (_cooldown_timer > 0.0f)
	{
		_cooldown_timer = max(_cooldown_timer - dt, 0.0f);
		return;
	}

	spawnOne();
}

bool RandomSpawner::spawnOne()
{
	const int index = pickTemplateIndex();
	if (index < 0)
		return false;

	NodePtr spawned = World::loadNode(spawnNodes[index]);
	if (!spawned)
		return false;

	spawned->setWorldTransform(getSpawnTransform());
	spawned->setEnabled(true);

	if (cooldownAfterDeathOnly)
	{
		// Always track the spawned node. Entity lookup may transiently miss
		// (init order, FLOGERR, registration timing) — isActiveSpawnAlive falls
		// back to node-existence so a failed lookup never causes a double spawn.
		_active_spawn = spawned;
	}

	return true;
}

int RandomSpawner::pickTemplateIndex()
{
	Vector<int> indices;
	for (int i = 0; i < spawnNodes.size(); ++i)
	{
		if (spawnNodes[i].get())
			indices.append(i);
	}
	if (indices.empty())
		return -1;

	const float r01 = clamp(Game::getRandomFloat(0.0f, 1.0f), 0.0f, 1.0f);
	int pick = (int)(r01 * (float)indices.size());
	if (pick >= indices.size())
		pick = indices.size() - 1;

	return indices[pick];
}

bool RandomSpawner::isActiveSpawnAlive() const
{
	if (!_active_spawn || _active_spawn.isDeleted())
		return false;

	// Lazy entity lookup — registration / init order can make it transiently
	// nullptr right after loadNode. If we ever do find an Entity, trust its HP;
	// otherwise consider the spawn alive as long as the node still exists
	// (Entity self-deletes via deleteTimer when persistOnDeath is false).
	if (Entity *entity = findEntityInHierarchy(_active_spawn))
		return entity->isAlive();
	return true;
}

Mat4 RandomSpawner::getSpawnTransform() const
{
	if (spawnPoint)
		return spawnPoint->getWorldTransform();
	if (node)
		return node->getWorldTransform();
	return Mat4_identity;
}
