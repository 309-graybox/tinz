#pragma once
#include <UnigineComponentSystem.h>

struct CameraPositionLag final: public Unigine::ComponentStruct
{
	PROP_PARAM(Toggle, separate_axis, false)

	PROP_PARAM(Double, speed, 1.0, "", "", "", "separate_axis=0")
	PROP_PARAM(Double, damping, 1.0, "", "", "", "separate_axis=0")
	PROP_PARAM(Double, max_distance, 300.0, "", "", "", "separate_axis=0")

	PROP_PARAM(DVec3, speed_axis, Unigine::Math::dvec3(1.0), "", "", "", "separate_axis=1")
	PROP_PARAM(DVec3, damping_axis, Unigine::Math::dvec3(1.0), "", "", "", "separate_axis=1")
	PROP_PARAM(DVec3, max_distance_axis, Unigine::Math::dvec3(300.0), "", "", "", "separate_axis=1")

	void init(const Unigine::Math::Vec3 &rawPos);
	Unigine::Math::Vec3 update(const Unigine::Math::Vec3 &rawPos, float ifps);

private:
	Unigine::Math::Vec3 _position;
	Unigine::Math::Vec3 _velocity;

	Unigine::Math::Vec3 _omega;
	Unigine::Math::Vec3 _expPiece;
};
