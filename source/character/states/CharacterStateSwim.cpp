#include "CharacterStateSwim.h"

void CharacterStateSwim::enter(CharacterState *previousState) {}

void CharacterStateSwim::update(Character *character) {}

void CharacterStateSwim::exit() {}

CharacterStateType CharacterStateSwim::type() const
{
    return CharacterStateType::Swim;
}
