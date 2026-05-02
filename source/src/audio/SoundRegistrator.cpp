#include "SoundRegistrator.h"

#include "SoundSettings.h"

#include <UnigineLog.h>

namespace audio
{

REGISTER_COMPONENT(SoundRegistrator)

using namespace Unigine;

void SoundRegistrator::init()
{
	const char *id = eventId.get();
	if (!id || !*id)
	{
		Log::warning(
			"SoundRegistrator: empty Event ID on node \"%s\" (id %d) — skipping registration\n",
			node ? node->getName() : "<null>", node ? node->getID() : -1);
		return;
	}

	SoundEvent e;
	e.sample = sample.get();

	NodePtr settingsNode = settings.get();
	SoundSettings *cfg = nullptr;
	if (settingsNode)
	{
		cfg = getComponent<SoundSettings>(settingsNode);
		if (!cfg)
			Log::warning(
				"SoundRegistrator: Settings node \"%s\" has no SoundSettings — falling back\n",
				settingsNode->getName());
	}
	if (!cfg)
		cfg = getComponent<SoundSettings>(node);
	if (cfg)
		cfg->applyTo(e);

	SoundManager::registerEvent(id, e);
	_registered_id = id;
}

void SoundRegistrator::shutdown()
{
	if (!_registered_id.empty())
	{
		SoundManager::unregisterEvent(_registered_id.get());
		_registered_id.clear();
	}
}

} // namespace audio
