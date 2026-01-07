#pragma once
#include "Ability.h"

class AbilityDash: public Ability
{
public:
	COMPONENT_DEFINE(AbilityDash, Ability)
	COMPONENT_INIT(init)
	COMPONENT_INIT(update)

	PROP_PARAM(Float, dashDistance, 5.0f)
	PROP_PARAM(Float, dashDuration, 0.05f)
	PROP_PARAM(Vec3, dashDirection, {0.0f, 0.0f, 1.0f})

	PROP_PARAM(Toggle, verticalMovement, false)
	PROP_PARAM(Toggle, cancelVerticalVelocity, false)

	PROP_PARAM(Float, timeScale, 1.0f)

	bool canBeUsed() const override;

protected:
	void useImpl() override;

private:
	void init();
	void update();

private:
	void start();
	void end();

private:
	float _dashTimer = 0.0f;
	Unigine::Math::vec3 _originalVelocity = {0.0f, 0.0f, 0.0f};
	float _originalTimeScale = 1.0f;
	bool _isDashing = false;
};
