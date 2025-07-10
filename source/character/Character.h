#pragma once
#include "character/CharacterState.h"

#include <UnigineHashMap.h>

struct CharacterInput
{};

class Character final
{
public:
    Character();
    ~Character();

    void changeState(CharacterStateType newState);
    void update();

private:
    CharacterState *_state;
    Unigine::HashMap<CharacterStateType, CharacterState *> _preloadStates;
};
