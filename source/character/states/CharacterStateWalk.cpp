#include "CharacterStateWalk.h"

void CharacterStateWalk::enter(CharacterState *previousState) {}

void CharacterStateWalk::update(Character *character) {}

void CharacterStateWalk::exit() {}

CharacterStateType CharacterStateWalk::type() const
{
    return CharacterStateType::Walk;
}
