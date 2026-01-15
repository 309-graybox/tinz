#pragma once
#include <UnigineComponentSystem.h>

struct CameraSpringArm final: public Unigine::ComponentStruct
{
	PROP_PARAM(Float, return_speed, 5.0f)
	PROP_PARAM(Float, return_delay, 0.2f)

	PROP_PARAM(Float, max_correction_distance, 3.0f)
	PROP_PARAM(Toggle, use_adaptive_speed, true)

	PROP_PARAM(Double, damping, 0.8f)

	void init(double initDist);
	float update(double desiredDist, double collisionDist, float ifps);

private:
	double _dist = 0.0;
	float _returnTimer = 0.0f;
	double _latchedDist = 0.0;
	bool _recovering = false;
};
