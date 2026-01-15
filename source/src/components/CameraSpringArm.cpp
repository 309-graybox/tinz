#include "CameraSpringArm.h"

using namespace Unigine;
using namespace Unigine::Math;

static inline double approach_exp(double current, double target, double lambda, float ifps)
{
	return current + (target - current) * (1.0f - expf(-lambda * ifps));
}

void CameraSpringArm::init(double initDist)
{
	_dist = initDist;
	_returnTimer = 0.0f;
	_latchedDist = 0.0;
	_recovering = false;
}

float CameraSpringArm::update(double desired, double allowed, float dt)
{
	const float eps = 0.02f;
	const bool colliding = (allowed < desired - eps);

	if (colliding)
	{
		if (_dist <= 0.0f)
			_dist = allowed;
		else
			_dist = Unigine::Math::min(_dist, allowed);

		_returnTimer = 0.0f;
		_latchedDist = _dist;
		_recovering = true;
		return _dist;
	}

	if (!_recovering)
	{
		_dist = desired;
		return _dist;
	}

	_returnTimer += dt;

	if (_returnTimer < return_delay)
	{
		_dist = _latchedDist;
		return _dist;
	}

	float k = return_speed;
	if (use_adaptive_speed)
	{
		float max_corr = (max_correction_distance > 1e-4f) ? max_correction_distance : 1.0f;
		float delta = desired - _dist;
		float s = clamp(delta / max_corr, 0.0f, 1.0f);
		k *= (0.5f + 1.5f * s);
	}

	k *= (1.0f - clamp(damping, 0.0, 1.0) * 0.75f);

	_dist = approach_exp(_dist, desired, k, dt);

	if (abs(_dist - desired) < 0.001)
	{
		_dist = desired;
		_recovering = false;
		_returnTimer = 0.0f;
	}

	return _dist;
}
