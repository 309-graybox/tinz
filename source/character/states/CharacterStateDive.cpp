#include "CharacterStateDive.h"

void CharacterStateDive::enter(CharacterState *previousState) {}

void CharacterStateDive::update(Character *character) {}

void CharacterStateDive::exit() {}

CharacterStateType CharacterStateDive::type() const
{
    return CharacterStateType::Dive;
}
