#include "CharacterStateRun.h"

void CharacterStateRun::enter(CharacterState *previousState) {}

void CharacterStateRun::update(Character *character) {}

void CharacterStateRun::exit() {}

CharacterStateType CharacterStateRun::type() const
{
    return CharacterStateType::Run;
}
