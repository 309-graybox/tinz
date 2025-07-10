#include "CharacterStateIdle.h"

void CharacterStateIdle::enter(CharacterState *previousState) {}

void CharacterStateIdle::update(Character *character) {}

void CharacterStateIdle::exit() {}

CharacterStateType CharacterStateIdle::type() const
{
    return CharacterStateType::Idle;
}
