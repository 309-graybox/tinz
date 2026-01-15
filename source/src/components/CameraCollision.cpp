#include "CameraCollision.h"
#include <UnigineVisualizer.h>

using namespace Unigine;
using namespace Unigine::Math;

Unigine::Math::Vec3 CameraCollision::update(const Vec3 &pos, const Vec3 &targetPos, float ifps)
{
	const auto desired = pos;

	auto dir = desired - targetPos;
	auto dist = length(dir);
	if (dist < 1e-4f)
		return desired;

	dir /= dist;

	auto right = cross(Vec3_up, dir);
	auto rl = length(right);
	right = rl > 1e-4f ? right / rl : Vec3(1.0f, 0.0f, 0.0f);

	auto up = normalize(cross(dir, right));

	auto offsets = {Vec3_zero, right * radius, -right * radius, up * radius, -up * radius};

	auto bestT = 1.0f;
	auto bestHit = desired;
	auto hitAny = false;
	auto bestOffset = Vec3_zero;

	WorldIntersectionPtr isect = WorldIntersection::create();

	for (const auto &off : offsets)
	{
		auto p0 = targetPos;
		auto p1 = desired + off;

		auto obj = World::getIntersection(p0, p1, mask, isect);
		if (!obj)
			continue;

		auto hp = isect->getPoint();
		auto t = length(hp - p0) / length(p1 - p0);

		if (t < bestT)
		{
			bestT = t;
			bestHit = hp;
			hitAny = true;
			bestOffset = off;
		}
	}

	if (!hitAny)
		return desired;

	Vec3 hit_center = targetPos + (desired - targetPos) * bestT;
	return hit_center - normalize(desired - targetPos) * (radius + offset);
}
