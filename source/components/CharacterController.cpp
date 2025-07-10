#include "CharacterController.h"

#include <UnigineInput.h>

void CharacterController::init()
{
    _body = node->getObjectBodyRigid();
    UNIGINE_ASSERT(_body);
}

void CharacterController::update()
{
    using Input = Unigine::Input;
    auto forward = Input::isKeyDown(Input::KEY_W) - Input::isKeyDown(Input::KEY_S);
    auto right = Input::isKeyDown(Input::KEY_D) - Input::isKeyDown(Input::KEY_A);
    auto nodeForward = node->getWorldDirection(Unigine::Math::AXIS_Y);
    auto nodeRight = node->getWorldDirection(Unigine::Math::AXIS_X);
}

void CharacterController::shutdown()
{
    _state = nullptr;
    for (auto &state : _states)
        delete state.data;
    _states.clear();
}
