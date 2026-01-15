#pragma once
#include <UnigineComponentSystem.h>

struct CameraCollision final: public Unigine::ComponentStruct
{
	PROP_PARAM(Double, radius, 0.3)
	PROP_PARAM(Double, offset, 0.01f)
	PROP_PARAM(Mask, mask, (int)0xffffffff)

	Unigine::Math::Vec3 update(const Unigine::Math::Vec3 &pos, const Unigine::Math::Vec3 &targetPos, float ifps);
};
