#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>

class Surface: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Surface, Unigine::ComponentBase)

	PROPERTY(String, surface_type, "default", Tooltip("Ключ поверхности; ищется в SurfaceRegistry"))

	const char *getType() { return surface_type.get(); }
};
