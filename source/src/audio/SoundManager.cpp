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

constexpr const char *kDefaultMusicLayer = "music";

struct Active3D
{
	SoundSourcePtr source;
};

struct PausedAmbient
{
	AmbientSourcePtr source;
	float pitch = 1.0f;
};

struct Paused3D
{
	SoundSourcePtr source;
	float pitch = 1.0f;
};

struct MusicStep
{
	SoundEvent event;
	String event_id;
	bool loop = false;
};

struct MusicStackEntry
{
	String previous_request;
	String override_request;
};

struct MusicLayer
{
	AmbientSourcePtr source;
	Vector<MusicStep> sequence;
	int sequence_index = 0;
	String current_request;
	Vector<MusicStackEntry> stack;
};

struct State
{
	bool initialized = false;
	bool enabled = true;
	bool paused = false;

	HashMap<String, SoundEvent> events;

	Vector<AmbientSourcePtr> active_2d;
	Vector<Active3D> active_3d;
	HashMap<String, MusicLayer> music_layers;
	Vector<PausedAmbient> paused_ambient;
	Vector<Paused3D> paused_3d;
	HashMap<String, float> last_play_time;
	float master_volume = 1;
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

// Returns true if the event is allowed to play and stamps the time.
// Returns false if still within min_interval since the last play.
bool consumeCooldown(const char *id_or_path, float min_interval)
{
	if (min_interval <= 0.0f || !id_or_path || !*id_or_path)
		return true;

	State &s = state();
	const String key(id_or_path);
	const float now = Game::getTime();
	if (s.last_play_time.contains(key))
	{
		const float last = s.last_play_time.get(key);
		if (now - last < min_interval)
			return false;
		s.last_play_time.get(key) = now;
	}
	else
	{
		s.last_play_time.append(key, now);
	}
	return true;
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

String normalizeMusicLayer(const char *layer)
{
	return (layer && *layer) ? layer : kDefaultMusicLayer;
}

MusicLayer &getMusicLayer(State &s, const char *layer)
{
	return s.music_layers.get(normalizeMusicLayer(layer));
}

void stopMusicSource(MusicLayer &layer)
{
	if (!layer.source)
		return;

	layer.source->stop();
	layer.source.deleteLater();
	layer.source.clear();
}

void stopMusicPlayback(MusicLayer &layer)
{
	stopMusicSource(layer);
	layer.sequence.clear();
	layer.sequence_index = 0;
}

void pauseAmbientSource(State &s, const AmbientSourcePtr &source)
{
	if (!source || source->isStopped())
		return;

	PausedAmbient rec;
	rec.source = source;
	rec.pitch = source->getPitch();
	s.paused_ambient.append(rec);
	source->setPitch(0.0f);
}

void pause3DSource(State &s, const SoundSourcePtr &source)
{
	if (!source || source->isStopped())
		return;

	Paused3D rec;
	rec.source = source;
	rec.pitch = source->getPitch();
	s.paused_3d.append(rec);
	source->setPitch(0.0f);
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

bool playMusicStep(MusicLayer &layer, const char *layer_name, int index)
{
	if (index < 0 || index >= (int)layer.sequence.size())
		return false;

	const MusicStep &step = layer.sequence[index];
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
		"SoundManager::playMusicLayer: layer=\"%s\" step %d/%d event=\"%s\" sample=\"%s\" loop=%d gain=%.3f mask=0x%08x\n",
		layer_name ? layer_name : "", index + 1, (int)layer.sequence.size(),
		step.event_id.get(), e.sample.get(), step.loop ? 1 : 0, gain, e.source_mask);

	layer.source = as;
	layer.sequence_index = index;
	return true;
}

void playNextMusicStep(MusicLayer &layer, const char *layer_name)
{
	const int previous_index = layer.sequence_index;
	stopMusicSource(layer);

	for (int i = layer.sequence_index + 1; i < (int)layer.sequence.size(); ++i)
	{
		Log::message(
			"SoundManager::playMusicLayer: layer=\"%s\" step %d finished, switching to step %d\n",
			layer_name ? layer_name : "", previous_index + 1, i + 1);
		if (playMusicStep(layer, layer_name, i))
			return;
	}

	if (!layer.sequence.empty())
		Log::message("SoundManager::playMusicLayer: layer=\"%s\" sequence finished\n",
			layer_name ? layer_name : "");
	layer.sequence.clear();
	layer.sequence_index = 0;
}

} // namespace

void SoundManager::init()
{
	ensureInitialized();
	State &s = state();
	s.events.clear();
	s.active_2d.clear();
	s.active_3d.clear();
	s.music_layers.clear();
	s.paused = false;
	s.paused_ambient.clear();
	s.paused_3d.clear();
	s.last_play_time.clear();
	Unigine::Sound::setVolume(s.master_volume);
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

	for (auto &it : s.music_layers)
		stopMusicPlayback(it.data);
	s.music_layers.clear();
	s.paused = false;
	s.paused_ambient.clear();
	s.paused_3d.clear();
	s.last_play_time.clear();

	s.events.clear();
	s.initialized = false;
}

void SoundManager::update()
{
	State &s = state();
	if (!s.initialized)
		return;

	if (s.paused)
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

	for (auto &it : s.music_layers)
	{
		MusicLayer &layer = it.data;
		if (layer.source && layer.source->isStopped())
			playNextMusicStep(layer, it.key.get());
	}
}

void SoundManager::setPaused(bool paused)
{
	ensureInitialized();
	State &s = state();
	if (s.paused == paused)
		return;

	s.paused = paused;
	if (paused)
	{
		for (auto &as : s.active_2d)
			pauseAmbientSource(s, as);

		for (auto &rec : s.active_3d)
			pause3DSource(s, rec.source);

		for (auto &it : s.music_layers)
			pauseAmbientSource(s, it.data.source);

		return;
	}

	for (auto &rec : s.paused_ambient)
	{
		if (rec.source && !rec.source->isStopped())
			rec.source->setPitch(rec.pitch);
	}
	s.paused_ambient.clear();

	for (auto &rec : s.paused_3d)
	{
		if (rec.source && !rec.source->isStopped())
			rec.source->setPitch(rec.pitch);
	}
	s.paused_3d.clear();
}

bool SoundManager::isPaused()
{
	return state().paused;
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

	if (!consumeCooldown(id_or_path, e->min_interval))
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

	if (!consumeCooldown(id_or_path, e->min_interval))
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
	playMusicLayer(kDefaultMusicLayer, id_or_path);
}

void SoundManager::stopMusic()
{
	stopMusicLayer(kDefaultMusicLayer);
}

void SoundManager::pushMusic(const char *id_or_path)
{
	pushMusicLayer(kDefaultMusicLayer, id_or_path);
}

void SoundManager::popMusic()
{
	popMusicLayer(kDefaultMusicLayer);
}

void SoundManager::playMusicLayer(const char *layer, const char *id_or_path)
{
	ensureInitialized();

	const String layer_name = normalizeMusicLayer(layer);
	SoundEvent fallback;
	const SoundEvent *e = resolveEvent(id_or_path, fallback);
	if (!e || (e->sample.empty() && e->loop_event_id.empty()))
	{
		Log::warning(
			"SoundManager::playMusicLayer: layer=\"%s\" empty or unresolved request \"%s\"\n",
			layer_name.get(), id_or_path ? id_or_path : "<null>");
		return;
	}

	const float gain = resolveGain(*e);
	if (gain <= 0.0f)
	{
		Log::message(
			"SoundManager::playMusicLayer: layer=\"%s\" request=\"%s\" skipped because gain is %.3f\n",
			layer_name.get(), id_or_path ? id_or_path : "<null>", gain);
		stopMusicLayer(layer_name.get());
		return;
	}

	Vector<MusicStep> sequence;
	Log::message(
		"SoundManager::playMusicLayer: layer=\"%s\" request=\"%s\" sample=\"%s\" loop_event_id=\"%s\"\n",
		layer_name.get(), id_or_path ? id_or_path : "<null>", e->sample.get(),
		e->loop_event_id.get());

	appendMusicStep(sequence, *e, id_or_path, e->sample.get(), e->loop_event_id.empty());
	appendResolvedMusicStep(sequence, e->loop_event_id.get(), true);
	if (sequence.empty())
	{
		Log::warning(
			"SoundManager::playMusicLayer: layer=\"%s\" request=\"%s\" has no playable steps\n",
			layer_name.get(), id_or_path ? id_or_path : "<null>");
		return;
	}

	State &s = state();
	MusicLayer &music_layer = getMusicLayer(s, layer_name.get());
	stopMusicPlayback(music_layer);

	music_layer.sequence = sequence;
	music_layer.current_request = id_or_path ? id_or_path : "";
	if (!playMusicStep(music_layer, layer_name.get(), 0))
		playNextMusicStep(music_layer, layer_name.get());
}

void SoundManager::stopMusicLayer(const char *layer)
{
	ensureInitialized();

	State &s = state();
	const String layer_name = normalizeMusicLayer(layer);
	MusicLayer &music_layer = getMusicLayer(s, layer_name.get());
	if (music_layer.source)
		Log::message("SoundManager::stopMusicLayer: layer=\"%s\"\n", layer_name.get());
	stopMusicPlayback(music_layer);
	music_layer.current_request.clear();
	music_layer.stack.clear();
}

void SoundManager::pushMusicLayer(const char *layer, const char *id_or_path)
{
	ensureInitialized();
	if (!id_or_path || !*id_or_path)
		return;

	const String layer_name = normalizeMusicLayer(layer);
	State &s = state();
	MusicLayer &music_layer = getMusicLayer(s, layer_name.get());

	MusicStackEntry entry;
	entry.previous_request = music_layer.current_request;
	entry.override_request = id_or_path;
	music_layer.stack.append(entry);

	playMusicLayer(layer_name.get(), id_or_path);
}

void SoundManager::popMusicLayer(const char *layer)
{
	ensureInitialized();

	const String layer_name = normalizeMusicLayer(layer);
	State &s = state();
	MusicLayer &music_layer = getMusicLayer(s, layer_name.get());
	if (music_layer.stack.empty())
		return;

	MusicStackEntry entry = music_layer.stack.last();
	music_layer.stack.removeLast();

	if (music_layer.current_request != entry.override_request)
	{
		Log::message(
			"SoundManager::popMusicLayer: layer=\"%s\" current music changed from override \"%s\" to \"%s\", not restoring \"%s\"\n",
			layer_name.get(), entry.override_request.get(), music_layer.current_request.get(),
			entry.previous_request.get());
		return;
	}

	if (entry.previous_request.empty())
	{
		stopMusicPlayback(music_layer);
		music_layer.current_request.clear();
		return;
	}

	playMusicLayer(layer_name.get(), entry.previous_request.get());
}

void SoundManager::setEnabled(bool enabled)
{
	state().enabled = enabled;
}

bool SoundManager::isEnabled()
{
	return state().enabled;
}

void SoundManager::setMasterVolume(float volume)
{
	auto &s = state();
	s.master_volume = volume;
	Unigine::Sound::setVolume(volume);
}

} // namespace audio
