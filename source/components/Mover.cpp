#include <UnigineComponentSystem.h>
#include <UnigineInput.h>
#include <UnigineConsole.h>
#include <UnigineGame.h>

using namespace Unigine;
using namespace Unigine::Math;

class Mover: public ComponentBase
{
public:
	COMPONENT_DEFINE(Mover, ComponentBase)
	COMPONENT_UPDATE(update)
	COMPONENT_UPDATE_PHYSICS(updatePhysics)

	PROP_PARAM(Float, speed, 3.0f)
	PROP_PARAM(Float, sprint_speed, 6.0f)
	PROP_PARAM(Float, jump, 7.5f)
	PROP_PARAM(Float, ground_angle_tolerance, 0.3f)
	PROP_PARAM(Mask, ground_mask, (int)0xffffffff)

	bool isOnGround() const
	{
		// TODO
		// It's shit
		// I know
		auto body = node->getObjectBodyRigid();
		return compare(body->getLinearVelocity().z, 0.0f, 0.1f);
	}

private:
	void update()
	{
		if (Console::isActive())
			return;

		mx = Input::isKeyPressed(Input::KEY_D) - Input::isKeyPressed(Input::KEY_A);
		my = Input::isKeyPressed(Input::KEY_W) - Input::isKeyPressed(Input::KEY_S);
		mz = Input::isKeyDown(Input::KEY_SPACE);
		ms = Input::isKeyPressed(Input::KEY_LEFT_SHIFT) ? sprint_speed : speed;

		auto player = Game::getPlayer();

		move = player->getWorldDirection(AXIS_X) * mx + player->getWorldDirection(AXIS_NZ) * my;
		move.z = 0.0f;
		move *= ms;

		if (node->getObjectBodyRigid())
			return;

		node->worldTranslate(Vec3(move * Game::getIFps()));
		move = vec3(0, 0, 0);
	}

	void updatePhysics()
	{
		if (!node->getObjectBodyRigid())
			return;

		auto body = node->getObjectBodyRigid();
		auto bv = body->getLinearVelocity();

		if (mz && isOnGround())
			move.z += jump;

		move.z += bv.z;
		body->setLinearVelocity(move);
		move = vec3(0, 0, 0);
	}

private:
	float mx = 0.0f;
	float my = 0.0f;
	float mz = 0.0f;
	float ms = 0.0f;

	vec3 move = {0, 0, 0};
};

REGISTER_COMPONENT(Mover)
