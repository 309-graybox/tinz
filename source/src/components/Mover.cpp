#include <UnigineComponentSystem.h>
#include <UnigineInput.h>
#include <UnigineConsole.h>
#include <UnigineGame.h>
#include "abilities/AbilityJump.h"
#include "abilities/AbilityDash.h"
#include "player/state/PlayerStateIdle.h"
#include "player/state/PlayerStateWalk.h"
#include "player/state/PlayerStateRun.h"
#include "player/state/PlayerStateCrouchIdle.h"
#include "player/state/PlayerStateCrouchWalk.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

using namespace Unigine;
using namespace Unigine::Math;

struct InputData
{
	bool forward : 1;
	bool backward : 1;
	bool left : 1;
	bool right : 1;
	bool up : 1;
	bool sprint : 1;
	bool dash : 1;
};

class Mover: public ComponentBase
{
public:
	COMPONENT_DEFINE(Mover, ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_UPDATE_PHYSICS(updatePhysics)

	PROP_PARAM(Float, speed, 8.0f)
	PROP_PARAM(Float, sprint_speed, 16.0f)
	PROP_PARAM(Float, rotation_speed, 1.0f)

private:
	void init()
	{
		_movement = 0;
		auto ms = checked_ptr_cast<ObjectMeshSkinned>(node);
		auto base = ms->addLayer();
		ms->setLayerAnimationFilePath(base, "character/animations/Breathing Idle.anim");
		ms->setLayer(base, true, 1.0f);

		_ctx.setMeshSkinned(ms);

		_rootState = new PlayerStateRoot;
		_currentState = _rootState;

		// Stand
		{
			auto stateIdle = new PlayerStateIdle(_ctx);
			_rootState->addChild(stateIdle);

			auto stateWalk = new PlayerStateWalk(_ctx, "move");
			stateIdle->addChild(stateWalk);

			auto stateRun = new PlayerStateRun(_ctx, "sprint");
			stateWalk->addChild(stateRun);
		}

		// Crouch
		{
			auto stateCrouchIdle = new PlayerStateCrouchIdle(_ctx, "crouch");
			_rootState->addChild(stateCrouchIdle);

			auto stateCrouchWalk = new PlayerStateCrouchWalk(_ctx, "move");
			stateCrouchIdle->addChild(stateCrouchWalk);
		}

		initEI();
	}

	void update()
	{
		if (Console::isActive())
			return;

		_ctx.update();

		_currentState->onUpdate(_ctx);
		auto newState = _rootState->transition();
		if (newState != _currentState)
		{
			_currentState->onExit(_ctx);
			_currentState = newState;
			_currentState->onEnter(_ctx);
		}
	}

	void updatePhysics()
	{
		//  node->getWorldDirection(AXIS_X)
		if (!node->getObjectBodyRigid())
			return;

		auto body = node->getObjectBodyRigid();

		if (_input.forward)
			body->addForce(node->getWorldDirection(AXIS_Y) * (_input.sprint ? sprint_speed : speed));
		if (_input.backward)
			body->addForce(node->getWorldDirection(AXIS_NY) * (_input.sprint ? sprint_speed : speed));
		if (_input.left)
			body->addTorque(node->getWorldDirection(AXIS_Z) * rotation_speed);
		if (_input.right)
			body->addTorque(node->getWorldDirection(AXIS_NZ) * rotation_speed);

		if (_input.up)
		{
			if (auto abilityJump = getComponent<AbilityJump>(node))
			{
				abilityJump->use();
			}
		}

		if (_input.dash)
		{
			if (auto abilityDash = getComponent<AbilityDash>(node))
			{
				auto dir = node->getWorldDirection(AXIS_Y);
				dir.z = 0;
				abilityDash->dashDirection = dir;
				abilityDash->use();
			}
		}

		_movement = 0;
	}

private:
	void initEI()
	{
		auto ei = EISystem::get();

		auto actionReg = ei->getActionRegistry();
		_actionMove = actionReg->create("move");
		_actionSprint = actionReg->create("sprint");
		_actionJump = actionReg->create("jump");
		_actionDash = actionReg->create("dash");
		_contextBase = ei->getContextRegistry()->create("base");

		ei->addContext(_contextBase);

		ei->bind(_actionMove, eTriggerState::Triggered, [&](EIActionValueInstance inst) {
			auto v = inst.getValue().value;
			_input.forward = v.y > 0;
			_input.backward = v.y < 0;
			_input.left = v.x < 0;
			_input.right = v.x > 0;
		});

		ei->bind(_actionSprint, eTriggerState::Triggered, [&](EIActionValueInstance inst) {
			auto v = inst.getValue().value;
			_input.sprint = !Math::compare(v.x, 0.0f);
		});

		ei->bind(_actionJump, eTriggerState::Triggered, [&](EIActionValueInstance inst) {
			auto v = inst.getValue().value;
			_input.up = !Math::compare(v.x, 0.0f);
		});

		ei->bind(_actionDash, eTriggerState::Triggered, [&](EIActionValueInstance inst) {
			auto v = inst.getValue().value;
			_input.dash = !Math::compare(v.x, 0.0f);
		});
	}

private:
	union
	{
		InputData _input;
		int _movement;
	};

	PlayerContext _ctx;
	PlayerState *_rootState = nullptr;
	PlayerState *_currentState = nullptr;

	EIAction *_actionMove = nullptr;
	EIAction *_actionSprint = nullptr;
	EIAction *_actionJump = nullptr;
	EIAction *_actionDash = nullptr;
	EIContext *_contextBase = nullptr;
};

REGISTER_COMPONENT(Mover)
