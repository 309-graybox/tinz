#include "CharacterStateLanding.h"

void CharacterStateLanding::enter(CharacterState *previousState) {}

void CharacterStateLanding::update(Character *character) {}

void CharacterStateLanding::exit() {}

CharacterStateType CharacterStateLanding::type() const
{
    return CharacterStateType::Landing;
}
