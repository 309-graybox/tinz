#pragma once
#include "character/CharacterState.h"

#include <UnigineComponentSystem.h>

class CharacterController : public Unigine::ComponentBase
{
public:
    COMPONENT_DEFINE(CharacterController, Unigine::ComponentBase);
    COMPONENT_INIT(init);
    COMPONENT_UPDATE(update);
    COMPONENT_SHUTDOWN(shutdown);

private:
    void init();
    void update();
    void shutdown();

private:
    Unigine::BodyRigidPtr _body;
    CharacterState *_state;
    Unigine::HashMap<CharacterStateType, CharacterState *> _states;
};
