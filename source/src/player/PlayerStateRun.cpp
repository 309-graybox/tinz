#include "PlayerStateRun.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

using namespace Unigine;
using namespace Unigine::Math;

PlayerStateRun::PlayerStateRun(PlayerContext &ctx, const char *actionName)
	: _sprint(false)
{
	auto ei = EISystem::get();
	auto action = ei->getActionRegistry()->create(actionName);

	ei->bind(action, eTriggerState::Triggered | eTriggerState::None, [&](EIActionValueInstance inst) {
		_sprint = !compare(inst.getValue().value.x, 0.0f);
	});


	auto ms = ctx.getMeshSkinned();
	auto l = ms->addLayer();
	setAnimationLayer(l);
	ms->setLayerAnimationFilePath(l, "character/animations/Running.anim");
	ms->setLayer(l, true, 0.0f);
}

float PlayerStateRun::getScore() const
{
	return _sprint ? 1.0f : -1.0f;
}
