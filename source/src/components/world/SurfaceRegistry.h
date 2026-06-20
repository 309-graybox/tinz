#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>

struct SurfaceConfig: Unigine::ComponentStruct
{
	PROPERTY(String, id, "default")
	PROPERTY(File, footmark_node, "")
	PROPERTY(Vec3, offset, Unigine::Math::vec3(0.0f, 0.0f, 0.01f))
	PROPERTY(String, step_sound, "")
};

class SurfaceRegistry: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(SurfaceRegistry, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_SHUTDOWN(shutdown)

	PROPERTY_ARRAY_STRUCT(SurfaceConfig, surfaces)

	static SurfaceConfig *find(const char *id);

private:
	void init();
	void shutdown();

private:
	static SurfaceRegistry *_instance;
};
