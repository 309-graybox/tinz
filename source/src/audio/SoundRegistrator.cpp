#include "SoundRegistrator.h"

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
	e.gain = gain;
	e.pitch_min = pitchMin;
	e.pitch_max = pitchMax;
	e.stream = ((int)stream) != 0;

	e.min_distance = minDistance;
	e.max_distance = maxDistance;
	e.air_absorption = airAbsorption;
	e.adaptation = adaptation;
	e.room_rolloff = roomRolloff;

	e.cone_inner_angle = coneInnerAngle;
	e.cone_outer_angle = coneOuterAngle;
	e.cone_outer_gain = coneOuterGain;
	e.cone_outer_gain_hf = coneOuterGainHF;

	e.source_mask = (int)sourceMask;
	e.reverb_mask = (int)reverbMask;
	e.occlusion_mask = (int)occlusionMask;
	e.occlusion = ((int)occlusion) != 0;

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
