#include "PlayerInput.h"
#include <UnigineGame.h>
#include <UnigineLog.h>

using namespace Unigine;
using namespace Unigine::Math;

void PlayerInput::init(const Unigine::NodePtr &node)
{
	auto ei = EISystem::get();
	if (!ei)
		return;

	auto player = ComponentSystem::get()->getComponent<EILocalPlayer>(node);
	if (!player)
		return;

	_node = node;

	auto action_reg = ei->getActionRegistry();

	auto _action_walk = action_reg->create("walk");
	auto _action_move = action_reg->create("move");
	auto _action_sprint = action_reg->create("sprint");
	auto _action_jump = action_reg->create("jump");
	auto _action_dash = action_reg->create("dash");
	auto _action_crouch = action_reg->create("crouch");
	auto _context_base = ei->getContextRegistry()->create("base");

	player->addContext(_context_base);

	_binding_walk = player->bind(_action_walk, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_walk = !compare(inst.getValue().value.x, 0.0f);
	});

	_binding_move = player->bind(_action_move, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_raw_move = inst.getValue().value.xy;
		if (abs(_raw_move.x) < 0.001)
			_raw_move.x = 0;
		if (abs(_raw_move.y) < 0.001)
			_raw_move.y = 0;
	});

	_binding_sprint = player->bind(_action_sprint, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_sprint = !compare(inst.getValue().value.x, 0.0f);
	});

	_binding_crouch = player->bind(_action_crouch, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_crouch = !compare(inst.getValue().value.x, 0.0f);
	});

	_binding_jump = player->bind(_action_jump, eTriggerState::Triggered, [this](EIActionValueInstance inst) {
		if (!compare(inst.getValue().value.x, 0.0f))
			_jump_requested = true;
	});

	_binding_dash = player->bind(_action_dash, eTriggerState::Triggered, [this](EIActionValueInstance inst) {
		if (!compare(inst.getValue().value.x, 0.0f))
			_dash_requested = true;
	});
}

void PlayerInput::update()
{
	_move_input.x = (_raw_move.x > 0) - (_raw_move.x < 0);
	_move_input.y = (_raw_move.y > 0) - (_raw_move.y < 0);

	_is_input_moving = _move_input.x != 0 || _move_input.y != 0;
}

void PlayerInput::shutdown()
{
	auto ei = EISystem::get();
	if (!ei)
		return;

	auto player = ComponentSystem::get()->getComponent<EILocalPlayer>(_node);
	if (!player)
		return;

	player->unbind(_binding_walk);
	player->unbind(_binding_move);
	player->unbind(_binding_sprint);
	player->unbind(_binding_crouch);
	player->unbind(_binding_jump);
	player->unbind(_binding_dash);
}

bool PlayerInput::consumeJump()
{
	return std::exchange(_jump_requested, false);
}

bool PlayerInput::consumeDash()
{
	return std::exchange(_dash_requested, false);
}
