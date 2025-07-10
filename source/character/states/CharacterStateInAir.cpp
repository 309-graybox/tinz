#include "CharacterStateInAir.h"

void CharacterStateInAir::enter(CharacterState *previousState) {}

void CharacterStateInAir::update(Character *character) {}

void CharacterStateInAir::exit() {}

CharacterStateType CharacterStateInAir::type() const
{
    return CharacterStateType::InAir;
}
