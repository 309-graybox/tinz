#pragma once
#include <UnigineMathLib.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class PlayerInput
{
public:
	void init(const Unigine::NodePtr &node);
	void update();
	void shutdown();

	Unigine::Math::vec2 getMoveInput() const noexcept { return _raw_move; }
	bool isInputMoving() const noexcept { return _is_input_moving; }
	bool isWalking() const noexcept { return _walk; }
	bool isSprinting() const noexcept { return _sprint; }
	bool isCrouching() const noexcept { return _crouch; }

	bool consumeJump();
	bool consumeDash();

private:
	Unigine::NodePtr _node;

	Unigine::Math::vec2 _move_input;
	Unigine::Math::vec2 _raw_move;
	bool _walk = false;
	bool _sprint = false;
	bool _crouch = false;
	bool _jump_requested = false;
	bool _dash_requested = false;
	bool _is_input_moving = false;

	EIBinding *_binding_walk = nullptr;
	EIBinding *_binding_move = nullptr;
	EIBinding *_binding_sprint = nullptr;
	EIBinding *_binding_crouch = nullptr;
	EIBinding *_binding_jump = nullptr;
	EIBinding *_binding_dash = nullptr;
};
