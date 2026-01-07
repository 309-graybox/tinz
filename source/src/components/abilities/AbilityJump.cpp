#include "AbilityJump.h"

REGISTER_COMPONENT(AbilityJump)

using namespace Unigine;
using namespace Unigine::Math;

bool AbilityJump::canBeUsed() const
{
	return Ability::canBeUsed() && node->getObjectBodyRigid() && (_isOnGround || _jump < inAirJumpsCount);
}

void AbilityJump::useImpl()
{
	auto body = node->getObjectBodyRigid();
	auto bv = body->getLinearVelocity();
	bv.z = 0;
	body->setLinearVelocity(bv);
	body->addLinearImpulse(vec3(0, 0, _jumpImpulse));
	if (!_isOnGround)
		++_jump;
}

void AbilityJump::init()
{
	_groudAngleToleranceCos = cos(groundAngleTolerance * Consts::DEG2RAD);
}

void AbilityJump::updatePhysics()
{
	_isOnGround = false;

	auto body = node->getObjectBodyRigid();
	_jumpImpulse = body->getMass() * sqrt(-2.0f * Physics::getGravity().z * jumpHeight);

	auto nContacts = body->getNumContacts();
	for (int i = 0; i < nContacts; ++i)
	{
		auto normal = body->getContactNormal(i);
		auto contactObject = body->getContactBody0(i);
		if (!contactObject || ((contactObject->getPhysicalMask() & groundMask) == 0))
			continue;

		float dotProduct = dot(normal, vec3_up);
		if (dotProduct > _groudAngleToleranceCos)
		{
			_isOnGround = true;
			break;
		}
	}

	if (_isOnGround)
		_jump = 0;
}
