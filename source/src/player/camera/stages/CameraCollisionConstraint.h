#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraCollisionConstraint final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraCollisionConstraint, CameraStageModifier);

	PROP_PARAM(Float, radius, 0.25f);
	PROP_PARAM(Float, extra_offset, 0.05f);
	PROP_PARAM(Toggle, use_diagonals, true);

	PROP_PARAM(Toggle, enable_spring, true);
	PROP_PARAM(Float, return_delay, 0.3f);
	PROP_PARAM(Float, return_speed, 1.0f);
	PROP_PARAM(Toggle, adaptive_speed, true);
	PROP_PARAM(Double, max_correction_dist, 1.0f);
	PROP_PARAM(Double, damping, 0.2f);

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	double updateDistance(double desired, double allowed, float dt);

private:
	Unigine::WorldIntersectionPtr _isect;

	double _dist = 0.0;
	float _returnTimer = 0.0f;
	double _latchedDist = 0.0;
	bool _recovering = false;
};
