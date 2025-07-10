#pragma once
#include "character/CharacterState.h"

class CharacterStateDive : public CharacterState
{
public:
    void enter(CharacterState *previousState) override;
    void update(Character *character) override;
    void exit() override;
    CharacterStateType type() const override;
};
