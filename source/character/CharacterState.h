#pragma once
#include "utils/defines.h"

ENUM_FLAG(CharacterStateType,
	Idle,
	Walk,
	Run,
	Crouch,
	Crawl,
	Dash,
	Attack,
	Jump,
	InAir,
	Landing,
	Swim,
	Dive,
	Dead);
ENUM_HASHER(CharacterStateType);

class Character;

class CharacterState
{
public:
	virtual ~CharacterState();
	virtual void enter(CharacterState *previousState);
	virtual void update(Character *character) = 0;
	virtual void exit();
	virtual CharacterStateType type() const = 0;
};
