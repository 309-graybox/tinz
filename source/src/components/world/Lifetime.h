#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>

class Lifetime: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Lifetime, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROPERTY(Toggle, use_time, true)
	PROPERTY(Float, time, 1.0f, Filter("use_time=1"))
	PROPERTY(Int, frames, 1, Filter("use_time=0"))

	UNIGINE_INLINE bool isTimed() const noexcept { return use_time.get(); }
	UNIGINE_INLINE bool isFramed() const noexcept { return !use_time.get(); }
	UNIGINE_INLINE float getRemainingTime() const noexcept { return _remainingTime; }
	UNIGINE_INLINE int getRemainingFrames() const noexcept { return _remainingFrames; }

protected:
	void init();
	void update();

private:
	float _remainingTime{0.0f};
	int _remainingFrames{0};
};
