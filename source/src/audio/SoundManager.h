#pragma once

#include <UnigineMathLib.h>
#include <UnigineNode.h>
#include <UnigineSounds.h>
#include <UnigineString.h>

namespace audio
{

// Per-event configuration. Mirrors the SoundSource attributes so anything you
// can set on a SoundSource node in the editor can also be expressed as a
// registered event. Lookup by string id; if no event is registered, playback
// functions accept the raw asset path directly so jam code can stay terse.
//
// Mixing buses are expressed via source_mask + Unigine::Sound::setSourceVolume
// — pick a bit per "category" and adjust its volume globally.
struct SoundEvent
{
	Unigine::String sample;          // .wav / .oga / .mp3

	// Mix.
	float gain = 1.0f;
	float pitch_min = 1.0f;
	float pitch_max = 1.0f;          // == pitch_min for no jitter
	bool stream = false;             // true for long files (music)

	// Spatialization (3D one-shots only).
	float min_distance = 1.0f;
	float max_distance = 100.0f;
	float air_absorption = 0.0f;     // 0..10, dB/m at high frequencies
	float adaptation = 0.0f;         // auto-volume adaptation factor
	float room_rolloff = 0.0f;       // reverb attenuation factor

	// Cone (3D one-shots only). Defaults = omni.
	float cone_inner_angle = 360.0f; // degrees
	float cone_outer_angle = 360.0f; // degrees
	float cone_outer_gain = 0.0f;
	float cone_outer_gain_hf = 1.0f;

	// Routing / occlusion.
	int source_mask = 0xffffffff;    // bit -> mixer source slot
	int reverb_mask = 0xffffffff;    // matched against SoundReverb zones
	int occlusion_mask = 0;          // physics intersection mask
	bool occlusion = false;          // enable occlusion test
};

// Lightweight, static, fire-and-forget audio facade. Designed for game jams:
// call play2D / play3DAt / playOnNode with a path or a registered event id and
// it just works. Built to grow: add buses, ducking, pooling, soft-limit per
// event, mixer config files etc. without changing call-sites.
class SoundManager
{
public:
	static void init();
	static void shutdown();

	// Call once per frame from world update. Cleans up finished one-shots.
	static void update();

	// Library — optional. If you don't register, callers pass the raw sample path.
	static void registerEvent(const char *id, const SoundEvent &event);
	static void registerEvent(const char *id, const char *sample, float gain = 1.0f,
		float pitch_jitter = 0.0f);
	static void unregisterEvent(const char *id);
	static bool hasEvent(const char *id);

	// One-shots. id_or_path is looked up in the library first, otherwise treated
	// as a direct sample path. Empty / null is a silent no-op.
	static void play2D(const char *id_or_path);
	static void play3DAt(const char *id_or_path, const Unigine::Math::Vec3 &world_pos);
	static void playOnNode(const char *id_or_path, const Unigine::NodePtr &node);

	// Global mute. While disabled, play* calls are silent no-ops. For per-bus
	// volume use Unigine::Sound::setSourceVolume(slot, vol) on the bits selected
	// by SoundEvent::source_mask. Master volume = Unigine::Sound::setVolume.
	static void setEnabled(bool enabled);
	static bool isEnabled();
};

} // namespace audio
