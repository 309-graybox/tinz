#include "AnimationState.h"
#include <UnigineGame.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

REGISTER_COMPONENT(AnimationState)

using namespace Unigine;
using namespace Unigine::Math;

void AnimationState::init()
{
	_ms = checked_ptr_cast<ObjectMeshSkinned>(node);
	UNIGINE_ASSERT(_ms);

	_ms->setNumLayers(5);

	_ms->setLayerAnimationFilePath(0, idle);
	_ms->setLayerAnimationFilePath(1, idle);
	_ms->setLayerAnimationFilePath(2, walk);
	_ms->setLayerAnimationFilePath(3, walk_reverse);
	_ms->setLayerAnimationFilePath(4, run);

	_ms->setLayer(0, true, 1.0f);
	_ms->setLayer(1, true, 0.0f);
	_ms->setLayer(2, true, 0.0f);
	_ms->setLayer(3, true, 0.0f);
	_ms->setLayer(4, true, 0.0f);

	_weight = 0.0f;
}

void AnimationState::update()
{
	_ms->setLayerFrame(0, Game::getTime() * 30.0f);
	_ms->setLayerFrame(1, Game::getTime() * 30.0f);
	_ms->setLayerFrame(2, Game::getTime() * 30.0f);
	_ms->setLayerFrame(3, Game::getTime() * 30.0f);
	_ms->setLayerFrame(4, Game::getTime() * 30.0f);

	_weight = clamp(_weight + Game::getIFps(), 0.0f, 1.0f);

	updateState();
}

void AnimationState::updateState()
{
	// if (Key::W.isDown() && Key::LeftShift.isDown())
	// 	changeState(State::Run);
	// else if (Key::W.isDown())
	// 	changeState(State::Walk);
	// else if (Key::S.isDown())
	// 	changeState(State::WalkReverse);
	// else
	// 	changeState(State::Idle);

	lerpState(_state);
}

void AnimationState::changeState(State to)
{
	if (_state == to)
		return;

	_weight = 0.0f;
	_prevState = _state;
	_state = to;
}

void AnimationState::lerpState(State to)
{
	_ms->lerpLayer(0, (int)_prevState + 1, (int)to + 1, _weight);
}
