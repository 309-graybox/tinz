#pragma once
#include <UnigineMathLib.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class PlayerInput
{
public:
	void init();
	void update(const Unigine::Math::vec3 &up = Unigine::Math::vec3_up);
	void shutdown();

	Unigine::Math::vec3 getMoveDirection() const noexcept { return _move_direction; }
	float getMoveAmount() const noexcept { return _move_amount; }
	bool isWalking() const noexcept { return _walk; }
	bool isSprinting() const noexcept { return _sprint; }
	bool isCrouching() const noexcept { return _crouch; }

	bool consumeJump();
	bool consumeDash();

private:
	Unigine::Math::vec3 _move_direction;
	float _move_amount = 0.0f;

	Unigine::Math::vec2 _raw_мove;
	bool _walk = false;
	bool _sprint = false;
	bool _crouch = false;
	bool _jump_requested = false;
	bool _dash_requested = false;

	EIAction *_action_walk = nullptr;
	EIAction *_action_move = nullptr;
	EIAction *_action_sprint = nullptr;
	EIAction *_action_crouch = nullptr;
	EIAction *_action_jump = nullptr;
	EIAction *_action_dash = nullptr;
	EIContext *_context_base = nullptr;

	EIBinding *_binding_walk = nullptr;
	EIBinding *_binding_move = nullptr;
	EIBinding *_binding_sprint = nullptr;
	EIBinding *_binding_crouch = nullptr;
	EIBinding *_binding_jump = nullptr;
	EIBinding *_binding_dash = nullptr;
};

