#include "PlayerStateIdle.h"

using namespace Unigine;
using namespace Unigine::Math;

PlayerStateIdle::PlayerStateIdle(PlayerContext &ctx)
{
	auto ms = ctx.getMeshSkinned();
	auto l = ms->addLayer();
	setAnimationLayer(l);
	ms->setLayerAnimationFilePath(l, "character/animations/Breathing Idle.anim");
	ms->setLayer(l, true, 0.0f);
}

float PlayerStateIdle::getScore() const
{
	return 0.0f;
}
