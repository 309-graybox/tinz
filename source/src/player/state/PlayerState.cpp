#include "PlayerState.h"
#include <UnigineLog.h>

using namespace Unigine;
using namespace Unigine::Math;

PlayerState::~PlayerState() = default;

void PlayerState::onEnter(PlayerContext &ctx)
{
	Log::message("Enter state %s\n", getStateName());
	onEnterImpl(ctx);
}

void PlayerState::onUpdate(PlayerContext &ctx)
{
	onUpdateImpl(ctx);
}

void PlayerState::onExit(PlayerContext &ctx)
{
	Log::message("Exit state %s\n", getStateName());
	onExitImpl(ctx);
}

PlayerState *PlayerState::transition()
{
	PlayerState *newState = nullptr;
	float maxScore = -1e6;
	for (auto &child : _children)
	{
		float childScore = child->getScore();
		if (childScore >= 0.0f && childScore > maxScore)
		{
			newState = child;
			maxScore = childScore;
		}
	}

	return newState ? newState->transition() : this;
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

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
PlayerState *PlayerStateRoot::transition()
{
	PlayerState *newState = nullptr;
	float maxScore = -1e6;
	for (auto &child : _children)
	{
		float childScore = child->getScore();
		if (childScore > maxScore)
		{
			newState = child;
			maxScore = childScore;
		}
	}

	return newState ? newState->transition() : this;
}
