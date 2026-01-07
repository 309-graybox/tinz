#include "PlayerStateIdle.h"

void PlayerStateIdle::onInitImpl(PlayerContext &ctx)
{
	auto ms = ctx.getMeshSkinned();
	auto base = ms->addLayer();
	ms->setLayerAnimationFilePath(base, "character/animations/Breathing Idle.anim");
	ms->setLayer(base, true, 1.0f);

	auto l = ms->addLayer();
	setAnimationLayer(l);
	ms->setLayerAnimationFilePath(l, "character/animations/Breathing Idle.anim");
	ms->setLayer(l, true, 0.0f);
}

void PlayerStateIdle::onEnterImpl(PlayerContext &ctx)
{
	ctx.setLayer(getAnimationLayer());
}

void PlayerStateIdle::onUpdateImpl(PlayerContext &ctx)
{
}

void PlayerStateIdle::onExitImpl(PlayerContext &ctx)
{
}
