#include "CharacterStateCrouch.h"

void CharacterStateCrouch::enter(CharacterState *previousState) {}

void CharacterStateCrouch::update(Character *character) {}

void CharacterStateCrouch::exit() {}

CharacterStateType CharacterStateCrouch::type() const
{
    return CharacterStateType::Crouch;
}
