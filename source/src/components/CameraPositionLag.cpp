#include "CameraPositionLag.h"

using namespace Unigine;
using namespace Unigine::Math;

void CameraPositionLag::init(const Unigine::Math::Vec3 &rawPos)
{
	_position = rawPos;
	_velocity = Vec3_zero;

	_omega = (!separate_axis ? Vec3(speed) : Vec3(speed_axis)) * Consts::PI2;
	_expPiece = -(!separate_axis ? Vec3(damping) : Vec3(damping_axis)) * _omega;
}

Unigine::Math::Vec3 CameraPositionLag::update(const Vec3 &rawPos, float ifps)
{
	auto expon = _expPiece * ifps;
	expon = Vec3(exp(expon.x), exp(expon.y), exp(expon.z));

	auto delta = _position - rawPos;
	auto temp = (_velocity + delta * _omega) * ifps;

	_velocity = (_velocity - temp * _omega) * expon;
	_position = rawPos + (delta + temp) * expon;

	return _position;
}
