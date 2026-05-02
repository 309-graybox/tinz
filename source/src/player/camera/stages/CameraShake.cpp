#include "CameraShake.h"

REGISTER_COMPONENT(CameraShake)

using namespace Unigine;
using namespace Unigine::Math;

void CameraShake::runtimeReset(CameraState &state, const CameraContext &)
{
	_phase = 0.0f;
	state.trauma = 0.0f;
}

void CameraShake::apply(CameraState &state, const CameraInput &, const CameraContext &)
{
	if (state.trauma <= 0.0f)
		return;

	_phase += state.dt;

	// trauma² → 0.5 trauma is a gentle wobble, 1.0 is dramatic. Linear scaling
	// makes low values feel uneventful; squared keeps a wider useful range.
	const float shake = state.trauma * state.trauma;
	const float omega = frequency * Consts::PI2;

	// Three sins with prime-ish frequency multipliers and phase offsets so the
	// axes don't co-oscillate (avoids the "everything orbits a circle" look).
	auto wobble = [&](float fmul, float phase) {
		return sin((_phase + phase) * omega * fmul);
	};

	// Apply offset in camera-local screen space — feels like the camera is
	// shaking, not the world drifting. Forward shake is muted so we don't
	// punch through the player or pop out behind them.
	vec3 right = state.rot * vec3_right;
	vec3 up    = state.rot * vec3_up;
	vec3 fwd   = state.rot * vec3_forward;

	vec3 offset = right * (wobble(1.00f, 0.13f) * shake * maxOffset)
	            + up    * (wobble(1.31f, 0.27f) * shake * maxOffset)
	            + fwd   * (wobble(0.79f, 0.41f) * shake * maxOffset * 0.4f);

	state.pos = state.pos + Vec3(offset);

	// Roll around the view forward axis — most "physical" feeling rotation
	// shake without messing up aim/pitch.
	float roll = wobble(1.13f, 0.55f) * shake * maxRotDeg.get();
	state.rot = state.rot * quat(vec3_forward, roll);

	state.trauma = max(0.0f, state.trauma - recoveryPerSecond * state.dt);
}
