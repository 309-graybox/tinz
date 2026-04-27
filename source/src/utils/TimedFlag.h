#pragma once
#include <UnigineGame.h>

class TimedFlag
{
public:
	void stamp() { _time = Unigine::Game::getTime(); }
	void clear() { _time = -1.0f; }

	bool isStamped() const { return _time >= 0.0f; }
	bool isFresh(float window) const
	{
		return isStamped() && Unigine::Game::getTime() - _time <= window;
	}
	bool consumeIfFresh(float window)
	{
		if (!isFresh(window))
			return false;
		clear();
		return true;
	}

	float getAge() const
	{
		return isStamped() ? Unigine::Game::getTime() - _time : -1.0f;
	}

private:
	float _time = -1.0f;
};
