#include "RandomSpawner.h"

#include "Entity.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(RandomSpawner)

using namespace Unigine;
using namespace Unigine::Math;

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

	NodePtr source = spawnNodes[index].get();
	if (!source)
		return false;

	NodePtr spawned = source->clone();
	if (!spawned)
	{
		Log::warning("RandomSpawner \"%s\": failed to clone node \"%s\"\n",
			node ? node->getName() : "", source->getName());
		return false;
	}

	spawned->setWorldTransform(getSpawnTransform());
	spawned->setEnabled(true);

	if (cooldownAfterDeathOnly)
	{
		Entity *entity = ComponentSystem::get()->getComponent<Entity>(spawned);
		if (entity && entity->isAlive())
		{
			_active_spawn = spawned;
		} else
		{
			// No Entity on spawned root: treat as instant-finished and run cooldown.
			_cooldown_timer = max((float)cooldown, 0.0f);
		}
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
	if (!_active_spawn)
		return false;

	Entity *entity = ComponentSystem::get()->getComponent<Entity>(_active_spawn);
	return entity && entity->isAlive();
}

Mat4 RandomSpawner::getSpawnTransform() const
{
	if (spawnPoint)
		return spawnPoint->getWorldTransform();
	if (node)
		return node->getWorldTransform();
	return Mat4_identity;
}
