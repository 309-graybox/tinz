#include "PlayerInput.h"
#include <UnigineGame.h>
#include <UnigineLog.h>

using namespace Unigine;
using namespace Unigine::Math;

void PlayerInput::init()
{
	auto ei = EISystem::get();
	auto action_reg = ei->getActionRegistry();

	_action_walk = action_reg->create("walk");
	_action_move = action_reg->create("move");
	_action_sprint = action_reg->create("sprint");
	_action_jump = action_reg->create("jump");
	_action_dash = action_reg->create("dash");
	_action_crouch = action_reg->create("crouch");
	_context_base = ei->getContextRegistry()->create("base");

	ei->addContext(_context_base);

	_binding_walk = ei->bind(_action_walk, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_walk = !compare(inst.getValue().value.x, 0.0f);
	});

	_binding_move = ei->bind(_action_move, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_raw_мove = inst.getValue().value.xy;
	});

	_binding_sprint = ei->bind(_action_sprint, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_sprint = !compare(inst.getValue().value.x, 0.0f);
	});

	_binding_crouch = ei->bind(_action_crouch, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance inst) {
		_crouch = !compare(inst.getValue().value.x, 0.0f);
	});

	_binding_jump = ei->bind(_action_jump, eTriggerState::Triggered, [this](EIActionValueInstance inst) {
		if (!compare(inst.getValue().value.x, 0.0f))
			_jump_requested = true;
	});

	_binding_dash = ei->bind(_action_dash, eTriggerState::Triggered, [this](EIActionValueInstance inst) {
		if (!compare(inst.getValue().value.x, 0.0f))
			_dash_requested = true;
	});
}

void PlayerInput::update(const Unigine::Math::vec3 &up)
{
	int forward = (_raw_мove.y > 0) - (_raw_мove.y < 0);
	int side = (_raw_мove.x > 0) - (_raw_мove.x < 0);

	if (forward == 0 && side == 0)
	{
		_move_direction = vec3_zero;
		_move_amount = 0.0f;
		return;
	}

	vec3 view_dir = Game::getPlayer()->getViewDirection();
	vec3 forward_dir = normalize(view_dir - vec3_up * dot(view_dir, vec3_up));
	vec3 right_dir = normalize(cross(forward_dir, vec3_up));
	vec3 move_dir = forward_dir * forward + right_dir * side;

	float d = dot(vec3_up, up);
	if (Math::abs(d) < 1e-6f)
		_move_direction = move_dir;
	else
		_move_direction = move_dir - vec3_up * (dot(move_dir, up) / d); // TODO(vah): try to remove division

	_move_direction.normalize();
	_move_amount = 1.0f;
}

void PlayerInput::shutdown()
{
	auto ei = EISystem::get();
	if (!ei)
		return;

	if (_binding_move) ei->unbind(_action_move, _binding_move);
	if (_binding_sprint) ei->unbind(_action_sprint, _binding_sprint);
	if (_binding_crouch) ei->unbind(_action_crouch, _binding_crouch);
	if (_binding_jump) ei->unbind(_action_jump, _binding_jump);
	if (_binding_dash) ei->unbind(_action_dash, _binding_dash);
}

bool PlayerInput::consumeJump()
{
	bool r = _jump_requested;
	_jump_requested = false;
	return r;
}

bool PlayerInput::consumeDash()
{
	bool r = _dash_requested;
	_dash_requested = false;
	return r;
}

