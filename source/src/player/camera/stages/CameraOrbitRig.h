#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraOrbitRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraOrbitRig, CameraStageModifier)

	void runtimeReset(CameraState &state, const CameraContext &ctx) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	void build(CameraState &state, const CameraContext &ctx) const;
};
