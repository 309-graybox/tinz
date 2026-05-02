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

struct MusicStep
{
	SoundEvent event;
	String event_id;
	bool loop = false;
};

struct State
{
	bool initialized = false;
	bool enabled = true;

	HashMap<String, SoundEvent> events;

	Vector<AmbientSourcePtr> active_2d;
	Vector<Active3D> active_3d;
	AmbientSourcePtr music;
	Vector<MusicStep> music_sequence;
	int music_sequence_index = 0;
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

const SoundEvent *resolveRegisteredEvent(const char *id)
{
	if (!id || !*id)
		return nullptr;

	State &s = state();
	String key(id);
	if (s.events.contains(key))
		return &s.events.get(key);

	return nullptr;
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

void stopMusicSource(State &s)
{
	if (!s.music)
		return;

	s.music->stop();
	s.music.deleteLater();
	s.music.clear();
}

void appendMusicStep(Vector<MusicStep> &sequence, const SoundEvent &event, const char *event_id,
	const char *sample, bool loop)
{
	if (!sample || !*sample)
		return;

	MusicStep step;
	step.event = event;
	step.event_id = event_id ? event_id : "";
	step.event.sample = sample;
	step.loop = loop;
	sequence.append(step);
}

void appendResolvedMusicStep(Vector<MusicStep> &sequence, const char *event_id, bool loop)
{
	if (!event_id || !*event_id)
		return;

	const SoundEvent *e = resolveRegisteredEvent(event_id);
	if (!e)
	{
		Log::warning("SoundManager::playMusic: loop event \"%s\" is not registered\n",
			event_id);
		return;
	}
	if (e->sample.empty())
	{
		Log::warning("SoundManager::playMusic: loop event \"%s\" has empty sample\n",
			event_id);
		return;
	}

	appendMusicStep(sequence, *e, event_id, e->sample.get(), loop);
}

bool playMusicStep(State &s, int index)
{
	if (index < 0 || index >= (int)s.music_sequence.size())
		return false;

	const MusicStep &step = s.music_sequence[index];
	const SoundEvent &e = step.event;
	const float gain = resolveGain(e);
	if (gain <= 0.0f)
		return false;

	AmbientSourcePtr as = AmbientSource::create(e.sample.get(), 1);
	if (!as)
	{
		Log::warning("SoundManager: failed to create AmbientSource for \"%s\"\n",
			e.sample.get());
		return false;
	}
	as->setGain(gain);
	as->setPitch(resolvePitch(e));
	as->setLoop(step.loop ? 1 : 0);
	as->setSourceMask(e.source_mask);
	as->play();

	Log::message(
		"SoundManager::playMusic: step %d/%d event=\"%s\" sample=\"%s\" loop=%d gain=%.3f mask=0x%08x\n",
		index + 1, (int)s.music_sequence.size(), step.event_id.get(), e.sample.get(),
		step.loop ? 1 : 0, gain, e.source_mask);

	s.music = as;
	s.music_sequence_index = index;
	return true;
}

void playNextMusicStep(State &s)
{
	const int previous_index = s.music_sequence_index;
	stopMusicSource(s);

	for (int i = s.music_sequence_index + 1; i < (int)s.music_sequence.size(); ++i)
	{
		Log::message("SoundManager::playMusic: step %d finished, switching to step %d\n",
			previous_index + 1, i + 1);
		if (playMusicStep(s, i))
			return;
	}

	if (!s.music_sequence.empty())
		Log::message("SoundManager::playMusic: sequence finished\n");
	s.music_sequence.clear();
	s.music_sequence_index = 0;
}

} // namespace

void SoundManager::init()
{
	ensureInitialized();
	State &s = state();
	s.events.clear();
	s.active_2d.clear();
	s.active_3d.clear();
	s.music_sequence.clear();
	s.music_sequence_index = 0;
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
		stopMusicSource(s);
	s.music_sequence.clear();
	s.music_sequence_index = 0;

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

	if (s.music && s.music->isStopped())
		playNextMusicStep(s);
}

void SoundManager::registerEvent(const char *id, const SoundEvent &event)
{
	if (!id || !*id)
		return;
	ensureInitialized();
	state().events.append(String(id), event);
	Log::message(
		"SoundManager::registerEvent: id=\"%s\" sample=\"%s\" loop_event_id=\"%s\" gain=%.3f mask=0x%08x stream=%d\n",
		id, event.sample.get(), event.loop_event_id.get(), event.gain, event.source_mask,
		event.stream ? 1 : 0);
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
	if (!e || (e->sample.empty() && e->loop_event_id.empty()))
	{
		Log::warning("SoundManager::playMusic: empty or unresolved request \"%s\"\n",
			id_or_path ? id_or_path : "<null>");
		return;
	}

	const float gain = resolveGain(*e);
	if (gain <= 0.0f)
	{
		Log::message("SoundManager::playMusic: request=\"%s\" skipped because gain is %.3f\n",
			id_or_path ? id_or_path : "<null>", gain);
		stopMusic();
		return;
	}

	Vector<MusicStep> sequence;
	Log::message("SoundManager::playMusic: request=\"%s\" sample=\"%s\" loop_event_id=\"%s\"\n",
		id_or_path ? id_or_path : "<null>", e->sample.get(), e->loop_event_id.get());

	appendMusicStep(sequence, *e, id_or_path, e->sample.get(), e->loop_event_id.empty());
	appendResolvedMusicStep(sequence, e->loop_event_id.get(), true);
	if (sequence.empty())
	{
		Log::warning("SoundManager::playMusic: request=\"%s\" has no playable steps\n",
			id_or_path ? id_or_path : "<null>");
		return;
	}

	stopMusic();

	State &s = state();
	s.music_sequence = sequence;
	if (!playMusicStep(s, 0))
		playNextMusicStep(s);
}

void SoundManager::stopMusic()
{
	State &s = state();
	if (s.music)
		Log::message("SoundManager::stopMusic\n");
	stopMusicSource(s);
	s.music_sequence.clear();
	s.music_sequence_index = 0;
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
