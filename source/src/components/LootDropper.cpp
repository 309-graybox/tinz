#include "LootDropper.h"

#include "Entity.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(LootDropper)

using namespace Unigine;
using namespace Unigine::Math;

void LootDropper::init()
{
	Entity *entity = ComponentSystem::get()->getComponent<Entity>(node);
	if (!entity)
	{
		Log::warning("LootDropper \"%s\": no Entity on the same node\n", node ? node->getName() : "");
		return;
	}

	entity->eventDied().connect(this, &LootDropper::onEntityDied);
}

void LootDropper::onEntityDied(Entity *entity)
{
	UNIGINE_UNUSED(entity)
	dropLoot();
}

void LootDropper::dropLoot()
{
	if (!node)
		return;

	const Vec3 origin = node->getWorldPosition();

	for (int i = 0; i < drops.size(); ++i)
	{
		auto &entry = drops[i];
		NodePtr drop_node = entry->dropNode.get();
		if (!drop_node)
			continue;
		if (!rollChance((float)entry->chance))
			continue;

		const int count = sampleCount((int)entry->minCount, (int)entry->maxCount);
		for (int n = 0; n < count; ++n)
		{
			NodePtr clone = drop_node->clone();
			if (!clone)
			{
				Log::warning("LootDropper \"%s\": failed to clone drop node \"%s\"\n",
					node->getName(), drop_node->getName());
				break;
			}

			clone->setWorldPosition(sampleDropPosition(origin));
			clone->setEnabled(true);
		}
	}
}

int LootDropper::sampleCount(int min_count_raw, int max_count_raw) const
{
	const int min_count = max(min_count_raw, 0);
	const int max_count = max(max_count_raw, min_count);

	if (max_count <= min_count)
		return min_count;

	const int span = max_count - min_count + 1;
	int pick = (int)(Game::getRandomFloat(0.0f, 1.0f) * (float)span);
	if (pick >= span)
		pick = span - 1;
	return min_count + pick;
}

bool LootDropper::rollChance(float chance_value) const
{
	// Supports both 0..1 and 0..100 authoring styles.
	float chance01 = chance_value;
	if (chance01 > 1.0f)
		chance01 *= 0.01f;
	chance01 = clamp(chance01, 0.0f, 1.0f);

	return Game::getRandomFloat(0.0f, 1.0f) <= chance01;
}

Vec3 LootDropper::sampleDropPosition(const Vec3 &origin) const
{
	const float radius = max((float)scatterRadius, 0.0f);
	const float height = max((float)scatterHeight, 0.0f);

	const float angle = Game::getRandomFloat(0.0f, Consts::PI2);
	const float dist = Game::getRandomFloat(0.0f, radius);
	const float up = Game::getRandomFloat(0.0f, height);

	return origin + Vec3(cos(angle) * dist, sin(angle) * dist, up);
}
