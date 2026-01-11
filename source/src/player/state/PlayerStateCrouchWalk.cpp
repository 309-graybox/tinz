#include "PlayerStateCrouchWalk.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

using namespace Unigine;
using namespace Unigine::Math;

PlayerStateCrouchWalk::PlayerStateCrouchWalk(PlayerContext &ctx, const char *actionName)
	: _move()
{
	auto ei = EISystem::get();
	auto action = ei->getActionRegistry()->create(actionName);

	ei->bind(action, eTriggerState::Triggered | eTriggerState::None, [&](EIActionValueInstance inst) {
		_move = inst.getValue().value;
	});

	auto ms = ctx.getMeshSkinned();
	auto l = ms->addLayer();
	setAnimationLayer(l);
	ms->setLayerAnimationFilePath(l, "character/animations/Crouched Walking.anim");
	ms->setLayer(l, true, 0.0f);
}

float PlayerStateCrouchWalk::getScore() const
{
	return !compare(length2(_move), 0.0f) ? 1.0f : -1.0f;
}
