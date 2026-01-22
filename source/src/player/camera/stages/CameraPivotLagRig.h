#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraPivotLagRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraPivotLagRig, CameraStageModifier)

	PROP_PARAM(Toggle, separate_axis, false)

	PROP_PARAM(Double, speed, 1.0, "", "", "", "separate_axis=0")
	PROP_PARAM(Double, damping, 1.0, "", "", "", "separate_axis=0")

	PROP_PARAM(DVec3, speed_axis, Unigine::Math::dvec3(1.0), "", "", "", "separate_axis=1")
	PROP_PARAM(DVec3, damping_axis, Unigine::Math::dvec3(1.0), "", "", "", "separate_axis=1")

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

	void init(const Unigine::Math::Vec3 &rawPos);
	Unigine::Math::Vec3 update(const Unigine::Math::Vec3 &rawPos, float ifps);

private:
	Unigine::Math::Vec3 _position;
	Unigine::Math::Vec3 _velocity;

	Unigine::Math::Vec3 _omega;
	Unigine::Math::Vec3 _expPiece;
};
