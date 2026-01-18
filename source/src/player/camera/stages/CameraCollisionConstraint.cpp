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
	float desired_dist = seg.length();
	if (desired_dist < 1e-4f)
		return;

	Vec3 dir = seg / desired_dist;

	Vec3 cam_right = normalize(state.rot * Vec3(1, 0, 0));
	Vec3 cam_up = normalize(state.rot * Vec3(0, 0, 1));

	const float r = radius.get();
	const float diag = r * 0.70710678f;

	Unigine::Vector<Vec3> offsets;
	offsets.append(Vec3_zero);
	offsets.append(cam_right * r);
	offsets.append(-cam_right * r);
	offsets.append(cam_up * r);
	offsets.append(-cam_up * r);

	if (use_diagonals.get())
	{
		offsets.append(cam_right * diag + cam_up * diag);
		offsets.append(cam_right * diag - cam_up * diag);
		offsets.append(-cam_right * diag + cam_up * diag);
		offsets.append(-cam_right * diag - cam_up * diag);
	}

	float allowed = desired_dist;

	for (const Vec3 &off : offsets)
	{
		Vec3 p0 = pivot;
		Vec3 p1 = desired + off;

		auto obj = World::getIntersection(p0, p1, ctx.collision_mask, _isect);
		if (!obj)
			continue;

		float hit_dist = length(_isect->getPoint() - pivot);

		float a = hit_dist - (r + extra_offset.get());
		if (a < allowed)
		{
			allowed = a;
		}
	}

	allowed = clamp(allowed, 0.0f, desired_dist);

	double out_dist = allowed;

	if (enable_spring.get())
	{
		out_dist = updateDistance(desired_dist, allowed, state.dt);
	}

	state.pos = pivot + dir * float(out_dist);
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
