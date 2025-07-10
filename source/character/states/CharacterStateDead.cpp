#include "CharacterStateDead.h"

void CharacterStateDead::enter(CharacterState *previousState) {}

void CharacterStateDead::update(Character *character) {}

void CharacterStateDead::exit() {}

CharacterStateType CharacterStateDead::type() const
{
    return CharacterStateType::Dead;
}
