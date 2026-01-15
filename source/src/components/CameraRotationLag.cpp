#include "CameraRotationLag.h"

using namespace Unigine;
using namespace Unigine::Math;

void CameraRotationLag::init(const Unigine::Math::vec2 &rawAngle)
{
	_angle = rawAngle;
	_velocity = vec2_zero;

	auto s = (!separate_axis ? vec2(speed) : speed_axis) * 2.0f;
	_omega = vec2(max(s.x, 0.001f), max(s.y / 2.5f, 0.001f));
}

Unigine::Math::vec2 CameraRotationLag::update(const Unigine::Math::vec2 &rawAngle, float ifps)
{
	auto x = _omega * ifps;
	auto x2 = x * x;
	auto exp = vec2_one / (vec2_one + x + x2 * 0.48f + x2 * x * 0.235f);

	auto change = _angle - rawAngle;
	auto temp = (_velocity + _omega * change) * ifps;
	_velocity = (_velocity - _omega * temp) * exp;

	_angle = rawAngle + (change + temp) * exp;

	return _angle;
}
