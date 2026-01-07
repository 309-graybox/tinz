#include "PlayerState.h"
#include <UnigineLog.h>

using namespace Unigine;

PlayerState::~PlayerState() = default;

void PlayerState::onInit(PlayerContext &ctx)
{
	Log::message("Init state %s\n", getStateName());
	onInitImpl(ctx);
}

void PlayerState::onEnter(PlayerContext &ctx)
{
	Log::message("Enter state %s\n", getStateName());
	onEnterImpl(ctx);
}

void PlayerState::onUpdate(PlayerContext &ctx)
{
	// Log::message("Update state %s\n", getStateName());
	onUpdateImpl(ctx);
}

void PlayerState::onExit(PlayerContext &ctx)
{
	Log::message("Exit state %s\n", getStateName());
	onExitImpl(ctx);
}

PlayerState *PlayerState::getParent() noexcept
{
	return _parent;
}

const PlayerState *PlayerState::getParent() const noexcept
{
	return _parent;
}

void PlayerState::setParent(PlayerState *parent)
{
	if (_parent == parent)
		return;

	if (_parent)
		_parent->removeChild(this);

	_parent = parent;

	if (_parent)
		_parent->addChild(parent);
}

void PlayerState::addChild(PlayerState *child)
{
	if (child && !_children.contains(child))
	{
		_children.append(child);

		if (child->_parent)
			child->_parent->removeChild(child);

		child->_parent = this;
	}
}

void PlayerState::removeChild(PlayerState *child)
{
	if (child)
	{
		_children.remove(_children.find(child));
		child->_parent = nullptr;
	}
}

const Unigine::Vector<PlayerState *> &PlayerState::getChildren() const noexcept
{
	return _children;
}
