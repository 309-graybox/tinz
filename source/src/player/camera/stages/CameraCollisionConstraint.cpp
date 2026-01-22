#include "CameraCollisionConstraint.h"

REGISTER_COMPONENT(CameraCollisionConstraint)

using namespace Unigine;
using namespace Unigine::Math;

static inline double approach_exp(double current, double target, double lambda, float ifps)
{
	return current + (target - current) * (1.0f - expf(-lambda * ifps));
}

void CameraCollisionConstraint::runtimeReset(CameraState &state, const CameraContext &ctx)
{
	_isect = WorldIntersection::create();
	_dist = length(dvec3(state.pos) - dvec3(state.rig.pivot));
	_returnTimer = 0.0f;
	_latchedDist = 0.0;
	_recovering = false;
}

void CameraCollisionConstraint::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	const Vec3 pivot = state.rig.pivot;
	const Vec3 desired = state.pos;

	Vec3 seg = desired - pivot;
	Scalar desired_dist = seg.length();
	if (desired_dist < 1e-4f)
		return;

	Vec3 dir = seg / desired_dist;

	Vec3 camRight = normalize(state.rot * Vec3_right);
	Vec3 camUp = normalize(state.rot * Vec3_up);

	const float r = radius.get();

	Unigine::Vector<Vec3> offsets{Vec3_zero, camRight * r, -camRight * r, camUp * r, -camUp * r};

	if (use_diagonals.get())
	{
		const float diag = r * 0.70710678f;

		offsets.append(camRight * diag + camUp * diag);
		offsets.append(camRight * diag - camUp * diag);
		offsets.append(-camRight * diag + camUp * diag);
		offsets.append(-camRight * diag - camUp * diag);
	}

	Scalar allowed = desired_dist;

	for (const Vec3 &off : offsets)
	{
		if (World::getIntersection(pivot, desired + off, ctx.collision_mask, _isect))
			allowed = min(allowed, length(_isect->getPoint() - pivot) - (r + extra_offset.get()));
	}

	allowed = clamp(allowed, Scalar(0), Scalar(desired_dist));

	Scalar outDist = allowed;
	if (enable_spring.get())
		outDist = updateDistance(desired_dist, allowed, state.dt);

	state.pos = pivot + dir * outDist;
}

double CameraCollisionConstraint::updateDistance(double desired, double allowed, float dt)
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
	if (adaptive_speed)
	{
		float max_corr = (max_correction_dist > 1e-4f) ? max_correction_dist : 1.0f;
		float s = clamp((desired - _dist) / max_corr, Scalar(0), Scalar(1));
		k *= (0.5f + 1.5f * s);
	}

	k *= (1.0f - clamp(damping, Scalar(0), Scalar(1)) * 0.75);

	_dist = approach_exp(_dist, desired, k, dt);

	if (abs(_dist - desired) < 0.001)
	{
		_dist = desired;
		_recovering = false;
		_returnTimer = 0.0f;
	}

	return _dist;
}
