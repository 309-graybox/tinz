#pragma once
#include "MovementState.h"

#include <UnigineMathLibVec3.h>

class SlideState : public MovementState
{
public:
	void init();
	void onEnter(MovementContext &ctx) override;
	MovementStateIndex update(MovementContext &ctx, float ifps) override;

	const char *name() const override { return "Slide"; }

private:
	float _slide_speed = 0.0f;
	Unigine::Math::vec3 _slide_base = Unigine::Math::vec3_zero;
};
