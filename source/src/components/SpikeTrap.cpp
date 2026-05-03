#include "SpikeTrap.h"

#include "Entity.h"
#include "game/GameState.h"
#include "utils/Utils.h"

#include <UnigineGame.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(SpikeTrap)

using namespace Unigine;
using namespace Unigine::Math;

void SpikeTrap::init()
{
	_trigger = checked_ptr_cast<WorldTrigger>(trigger.get());
	FLOGERR(_trigger, "trigger should be WorldTrigger\n");

	_trigger->setTriggerInteractionEnabled(true);
	_trigger->getEventEnter().connect(this, &SpikeTrap::onEnter);
	_trigger->getEventLeave().connect(this, &SpikeTrap::onLeave);
}

void SpikeTrap::update()
{
	if (_cooldownTimer > 0.0f)
		_cooldownTimer = max(_cooldownTimer - Game::getIFps(), 0.0f);

	if (active && repeatWhileInside && _playerInside)
		tryDamagePlayer();
}

void SpikeTrap::onEnter(const NodePtr &n)
{
	if (!isPlayerNode(n))
		return;

	_playerInside = true;
	tryDamagePlayer();
}

void SpikeTrap::onLeave(const NodePtr &n)
{
	if (!isPlayerNode(n))
		return;

	_playerInside = false;
}

bool SpikeTrap::isPlayerNode(const NodePtr &n) const
{
	NodePtr player = game::GameState::getPlayerCharacter();
	return player && isInHierarchy(n, player);
}

bool SpikeTrap::tryDamagePlayer()
{
	if (!active || _cooldownTimer > Consts::EPS)
		return false;

	NodePtr player = game::GameState::getPlayerCharacter();
	if (!player)
		return false;

	Entity *entity = ComponentSystem::get()->getComponent<Entity>(player);
	if (!entity || entity->isDead())
		return false;

	bool applied = entity->takeDamage(makeDamageInfo(node, "spikes", damage));
	_cooldownTimer = max(cooldown.get(), 0.0f);

	return applied;
}
