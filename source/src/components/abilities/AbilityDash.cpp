#include "AbilityDash.h"
#include <UnigineGame.h>

REGISTER_COMPONENT(AbilityDash)

using namespace Unigine;
using namespace Unigine::Math;

bool AbilityDash::canBeUsed() const
{
	return Ability::canBeUsed() && node->getObjectBodyRigid();
}

void AbilityDash::useImpl()
{
	start();
}

void AbilityDash::init()
{
	_originalTimeScale = Game::getScale();
}

void AbilityDash::update()
{
	if (!_isDashing)
		return;

	_dashTimer -= Game::getIFps();

	if (_dashTimer <= 0.0f)
	{
		end();
		return;
	}
}

void AbilityDash::start()
{
	auto body = node->getObjectBodyRigid();

	_isDashing = true;
	_dashTimer = dashDuration;
	_originalVelocity = body->getLinearVelocity();
	_originalTimeScale = Game::getScale();
	Game::setScale(timeScale);

	if (!verticalMovement)
		dashDirection = vec3(dashDirection.get().x, dashDirection.get().y, 0.0f);

	body->addLinearImpulse(dashDirection * body->getMass() * (dashDistance / dashDuration));
}

void AbilityDash::end()
{
	auto body = node->getObjectBodyRigid();

	_isDashing = false;
	_dashTimer = 0.0f;
	Game::setScale(_originalTimeScale);

	body->setLinearVelocity(vec3_zero);
}
