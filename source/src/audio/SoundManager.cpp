#include "SoundManager.h"

#include <UnigineGame.h>
#include <UnigineHashMap.h>
#include <UnigineLog.h>
#include <UnigineVector.h>

namespace audio
{

using namespace Unigine;

namespace
{

struct Active3D
{
	SoundSourcePtr source;
};

struct State
{
	bool initialized = false;
	bool enabled = true;

	HashMap<String, SoundEvent> events;

	Vector<AmbientSourcePtr> active_2d;
	Vector<Active3D> active_3d;
	AmbientSourcePtr music;
};

State &state()
{
	static State s;
	return s;
}

void ensureInitialized()
{
	State &s = state();
	if (!s.initialized)
	{
		s.initialized = true;
		s.enabled = true;
	}
}

float resolveGain(const SoundEvent &e)
{
	const State &s = state();
	if (!s.enabled)
		return 0.0f;
	return e.gain;
}

float resolvePitch(const SoundEvent &e)
{
	if (e.pitch_max <= e.pitch_min)
		return e.pitch_min;
	return Game::getRandomFloat(e.pitch_min, e.pitch_max);
}

// Returns nullptr if id_or_path is empty. Otherwise: registered event if it
// exists, else a synthetic event whose sample is the raw path.
const SoundEvent *resolveEvent(const char *id_or_path, SoundEvent &fallback)
{
	if (!id_or_path || !*id_or_path)
		return nullptr;

	State &s = state();
	String key(id_or_path);
	if (s.events.contains(key))
		return &s.events.get(key);

	fallback = SoundEvent{};
	fallback.sample = id_or_path;
	return &fallback;
}

void apply3DSpatialization(const SoundSourcePtr &src, const SoundEvent &e)
{
	src->setMinDistance(e.min_distance);
	src->setMaxDistance(e.max_distance);
	src->setAirAbsorption(e.air_absorption);
	src->setAdaptation(e.adaptation);
	src->setRoomRolloff(e.room_rolloff);

	src->setConeInnerAngle(e.cone_inner_angle);
	src->setConeOuterAngle(e.cone_outer_angle);
	src->setConeOuterGain(e.cone_outer_gain);
	src->setConeOuterGainHF(e.cone_outer_gain_hf);

	src->setSourceMask(e.source_mask);
	src->setReverbMask(e.reverb_mask);
	src->setOcclusionMask(e.occlusion_mask);
	src->setOcclusion(e.occlusion ? 1 : 0);
}

} // namespace

void SoundManager::init()
{
	ensureInitialized();
	State &s = state();
	s.events.clear();
	s.active_2d.clear();
	s.active_3d.clear();
}

void SoundManager::shutdown()
{
	State &s = state();

	for (auto &as : s.active_2d)
	{
		if (as)
		{
			as->stop();
			as.deleteLater();
		}
	}
	s.active_2d.clear();

	for (auto &rec : s.active_3d)
	{
		if (rec.source)
		{
			rec.source->stop();
			rec.source.deleteLater();
		}
	}
	s.active_3d.clear();

	if (s.music)
	{
		s.music->stop();
		s.music.deleteLater();
		s.music.clear();
	}

	s.events.clear();
	s.initialized = false;
}

void SoundManager::update()
{
	State &s = state();
	if (!s.initialized)
		return;

	for (int i = (int)s.active_2d.size() - 1; i >= 0; --i)
	{
		auto &as = s.active_2d[i];
		if (!as || as->isStopped())
		{
			if (as)
				as.deleteLater();
			s.active_2d.removeFast(i);
		}
	}

	for (int i = (int)s.active_3d.size() - 1; i >= 0; --i)
	{
		auto &rec = s.active_3d[i];
		if (!rec.source || rec.source->isStopped())
		{
			if (rec.source)
				rec.source.deleteLater();
			s.active_3d.removeFast(i);
		}
	}
}

void SoundManager::registerEvent(const char *id, const SoundEvent &event)
{
	if (!id || !*id)
		return;
	ensureInitialized();
	state().events.append(String(id), event);
}

void SoundManager::registerEvent(const char *id, const char *sample, float gain,
	float pitch_jitter)
{
	SoundEvent e;
	e.sample = sample ? sample : "";
	e.gain = gain;
	if (pitch_jitter > 0.0f)
	{
		e.pitch_min = 1.0f - pitch_jitter;
		e.pitch_max = 1.0f + pitch_jitter;
	}
	registerEvent(id, e);
}

void SoundManager::unregisterEvent(const char *id)
{
	if (!id || !*id)
		return;
	State &s = state();
	if (!s.initialized)
		return;
	String key(id);
	if (s.events.contains(key))
		s.events.remove(key);
}

bool SoundManager::hasEvent(const char *id)
{
	if (!id || !*id)
		return false;
	return state().events.contains(String(id));
}

void SoundManager::play2D(const char *id_or_path)
{
	ensureInitialized();

	SoundEvent fallback;
	const SoundEvent *e = resolveEvent(id_or_path, fallback);
	if (!e || e->sample.empty())
		return;

	const float gain = resolveGain(*e);
	if (gain <= 0.0f)
		return;

	AmbientSourcePtr as = AmbientSource::create(e->sample.get(), e->stream ? 1 : 0);
	if (!as)
	{
		Log::warning("SoundManager::play2D: failed to create AmbientSource for \"%s\"\n",
			e->sample.get());
		return;
	}
	as->setGain(gain);
	as->setPitch(resolvePitch(*e));
	as->setLoop(0);
	as->setSourceMask(e->source_mask);
	as->play();

	state().active_2d.append(as);
}

void SoundManager::play3DAt(const char *id_or_path, const Math::Vec3 &world_pos)
{
	ensureInitialized();

	SoundEvent fallback;
	const SoundEvent *e = resolveEvent(id_or_path, fallback);
	if (!e || e->sample.empty())
	{
		Log::error("Can not resolve sound event '%s'\n", id_or_path);
		return;
	}

	const float gain = resolveGain(*e);
	if (gain <= 0.0f)
		return;

	Log::message("Play \"%s\"\n", e->sample.get());

	SoundSourcePtr src = SoundSource::create(e->sample.get(), e->stream ? 1 : 0);
	if (!src)
	{
		Log::warning("SoundManager::play3DAt: failed to create SoundSource for \"%s\"\n",
			e->sample.get());
		return;
	}
	src->setWorldPosition(world_pos);
	src->setGain(gain);
	src->setPitch(resolvePitch(*e));
	src->setLoop(0);
	apply3DSpatialization(src, *e);

	src->play();

	Active3D rec;
	rec.source = src;
	state().active_3d.append(rec);
}

void SoundManager::playOnNode(const char *id_or_path, const NodePtr &node)
{
	if (!node)
		return;
	play3DAt(id_or_path, node->getWorldPosition());
}

void SoundManager::playMusic(const char *id_or_path)
{
	ensureInitialized();

	SoundEvent fallback;
	const SoundEvent *e = resolveEvent(id_or_path, fallback);
	if (!e || e->sample.empty())
		return;

	const float gain = resolveGain(*e);
	if (gain <= 0.0f)
	{
		stopMusic();
		return;
	}

	stopMusic();

	AmbientSourcePtr as = AmbientSource::create(e->sample.get(), 1);
	if (!as)
	{
		Log::warning("SoundManager::playMusic: failed to create AmbientSource for \"%s\"\n",
			e->sample.get());
		return;
	}
	as->setGain(gain);
	as->setPitch(resolvePitch(*e));
	as->setLoop(1);
	as->setSourceMask(e->source_mask);
	as->play();

	state().music = as;
}

void SoundManager::stopMusic()
{
	State &s = state();
	if (!s.music)
		return;
	s.music->stop();
	s.music.deleteLater();
	s.music.clear();
}

void SoundManager::setEnabled(bool enabled)
{
	state().enabled = enabled;
}

bool SoundManager::isEnabled()
{
	return state().enabled;
}

} // namespace audio
