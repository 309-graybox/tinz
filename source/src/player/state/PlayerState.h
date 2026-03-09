#pragma once
#include <UnigineObjects.h>
#include <UnigineGame.h>

#include "player/PlayerContext.h"

class PlayerState
{
public:
	virtual ~PlayerState();

	virtual const char *getStateName() const = 0;
	virtual float getScore() const = 0;

	int getAnimationLayer() const noexcept { return _animationLayer; }
	void setAnimationLayer(int layer) noexcept { _animationLayer = layer; }

	void onEnter(PlayerContext &ctx);
	void onUpdate(PlayerContext &ctx);
	void onExit(PlayerContext &ctx);

	virtual PlayerState *transition();

	PlayerState *getParent() noexcept;
	const PlayerState *getParent() const noexcept;
	void setParent(PlayerState *parent);

	void addChild(PlayerState *child);
	void removeChild(PlayerState *child);

	const Unigine::Vector<PlayerState *> &getChildren() const noexcept;

protected:
	virtual void onEnterImpl(PlayerContext &ctx)
	{
		auto l = getAnimationLayer();
		if (l != -1)
			ctx.setLayer(l);
	}
	virtual void onUpdateImpl(PlayerContext &ctx) {}
	virtual void onExitImpl(PlayerContext &ctx) {}

protected:
	PlayerState *_parent = nullptr;
	Unigine::Vector<PlayerState *> _children;

	int _animationLayer = -1;
};


class PlayerStateRoot: public PlayerState
{
public:
	const char *getStateName() const override { return "Root"; }
	float getScore() const override { return -1e6f; }

	PlayerState *transition() override;
};
