#pragma once
#include <UnigineComponentSystem.h>

class AnimationState: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(AnimationState, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	enum class State
	{
		Idle,
		Walk,
		WalkReverse,
		Run
	};

	PROP_PARAM(File, idle)
	PROP_PARAM(File, walk)
	PROP_PARAM(File, walk_reverse)
	PROP_PARAM(File, run)

private:
	void init();
	void update();

private:
	void updateState();
	void changeState(State to);
	void lerpState(State to);

private:
	Unigine::ObjectMeshSkinnedPtr _ms;
	float _weight;
	State _prevState = State::Idle;
	State _state = State::Idle;
};
