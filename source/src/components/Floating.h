#include <UnigineComponentSystem.h>

class Floating : public Unigine::ComponentBase
{
private:
	COMPONENT_DEFINE(Floating, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Float, amplitude, 0.1f);
	PROP_PARAM(Float, speed, 1.0f);
private:
	void init();
	void update();

	Unigine::Math::dvec3 _base_pos;
	float _time = 0.0f;
};
