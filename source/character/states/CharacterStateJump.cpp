#include "CharacterStateJump.h"

void CharacterStateJump::enter(CharacterState *previousState) {}

void CharacterStateJump::update(Character *character) {}

void CharacterStateJump::exit() {}

CharacterStateType CharacterStateJump::type() const
{
    return CharacterStateType::Jump;
}
