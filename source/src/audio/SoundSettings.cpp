#include "SoundSettings.h"

namespace audio
{

REGISTER_COMPONENT(SoundSettings)

void SoundSettings::applyTo(SoundEvent &e)
{
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
}

} // namespace audio
