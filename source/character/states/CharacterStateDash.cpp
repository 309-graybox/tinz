#include "CharacterStateDash.h"

void CharacterStateDash::enter(CharacterState *previousState) {}

void CharacterStateDash::update(Character *character) {}

void CharacterStateDash::exit() {}

CharacterStateType CharacterStateDash::type() const
{
    return CharacterStateType::Dash;
}
