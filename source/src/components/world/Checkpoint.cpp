#include "components/world/Checkpoint.h"
#include "utils/Utils.h"
#include "audio/SoundManager.h"
#include "game/GameState.h"

#include <UnigineLog.h>

REGISTER_COMPONENT(Checkpoint)

using namespace Unigine;
using namespace Unigine::Math;

void Checkpoint::init()
{
	_trigger = checked_ptr_cast<WorldTrigger>(trigger.get());
	if (!_trigger)
	{
		Log::error("Checkpoint \"%s\": trigger is not a WorldTrigger — disabling\n",
			node ? node->getName() : "<null>");
		return;
	}
	_trigger->getEventEnter().connect(this, &Checkpoint::onEnter);
}

void Checkpoint::onEnter(const NodePtr &n)
{
	if (_consumed && once)
		return;

	NodePtr playerChar = game::GameState::getPlayerCharacter();
	if (!playerChar)
		return;
	if (!isInHierarchy(n, playerChar))
		return;

	NodePtr anchor = spawn_anchor.get();
	Vec3 pos = anchor ? anchor->getWorldPosition() : _trigger->getWorldPosition();
	game::GameState::setLastCheckpoint(pos);

	_consumed = true;

	const char *snd = soundOnActivate.get();
	if (snd && *snd)
		audio::SoundManager::play2D(snd);
}
