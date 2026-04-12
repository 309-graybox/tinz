#pragma once
// #include "CharacterMovement.h"
#include <UnigineMathLibVec3.h>
#include "../input/PlayerInput.h"

class CharacterMovement;

struct MovementContext
{
	CharacterMovement *owner;
	
	// state read
	PlayerInput input;
	Unigine::Math::vec3 ground_normal;
	Unigine::Math::vec3 character_forward;
	Unigine::Math::vec3 desired_input_direction;
	bool is_grounded;

	// state write
	Unigine::Math::vec3 move_direction;
	Unigine::Math::vec3 rotate_target;
	float speed;
	float turn_speed;
	float vertical_impulse;
};