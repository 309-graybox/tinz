#include "PlayerStateCrouchIdle.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

using namespace Unigine;
using namespace Unigine::Math;

PlayerStateCrouchIdle::PlayerStateCrouchIdle(PlayerContext &ctx, const char *actionName)
	: _crouch(false)
{
	auto ei = EISystem::get();
	auto action = ei->getActionRegistry()->create(actionName);

	ei->bind(action, eTriggerState::Triggered | eTriggerState::None, [&](EIActionValueInstance inst) {
		_crouch = !compare(inst.getValue().value.x, 0.0f);
	});

	auto ms = ctx.getMeshSkinned();
	auto l = ms->addLayer();
	setAnimationLayer(l);
	ms->setLayerAnimationFilePath(l, "character/animations/Crouching Idle.anim");
	ms->setLayer(l, true, 0.0f);
}

float PlayerStateCrouchIdle::getScore() const
{
	return _crouch ? 1.0f : -1.0f;
}
