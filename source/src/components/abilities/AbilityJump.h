#pragma once
#include "Ability.h"

class AbilityJump: public Ability
{
public:
	COMPONENT_DEFINE(AbilityJump, Ability)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE_PHYSICS(updatePhysics)

	PROP_PARAM(Int, inAirJumpsCount, 0)

	PROP_PARAM(Float, jumpHeight, 4.0f)
	PROP_PARAM(Mask, groundMask, (int)0xffffffff)
	PROP_PARAM(Float, groundAngleTolerance, 30.0f)

	bool canBeUsed() const override;

protected:
	void useImpl() override;

private:
	void init();
	void updatePhysics();

private:
	bool _isOnGround = false;
	float _groudAngleToleranceCos = 0.0f;
	float _jumpImpulse = 0.0f;
	int _jump = 0;
};
