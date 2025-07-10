#include "Character.h"

Character::Character()
    : _state(nullptr)
    , _preloadStates()
{}

Character::~Character()
{
    _state = nullptr;
    for (auto &state : _preloadStates)
        delete state.data;
    _preloadStates.clear();
}

void Character::changeState(CharacterStateType newState)
{
    auto previousState = _state;
    _state->exit();
    _state = _preloadStates.get(newState);
    _state->enter(previousState);
}

void Character::update()
{
    _state->update(this);
}
