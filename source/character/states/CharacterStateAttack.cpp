#include "CharacterStateAttack.h"

void CharacterStateAttack::enter(CharacterState *previousState) {}

void CharacterStateAttack::update(Character *character) {}

void CharacterStateAttack::exit() {}

CharacterStateType CharacterStateAttack::type() const
{
    return CharacterStateType::Attack;
}
