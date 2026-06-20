#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>

class FootmarkPlacer: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(FootmarkPlacer, Unigine::ComponentBase)
	COMPONENT_UPDATE(update)

	PROPERTY_ND(File, node_to_place)
	PROPERTY(Mask, ground_mask, ~0)
	PROPERTY(Float, height, 0.2f)
	PROPERTY(Vec3, offset, Unigine::Math::vec3(0.0f, 0.0f, 0.1f))
	PROPERTY(Int, max_count, 10)

private:
	void update();

private:
	Unigine::WorldIntersectionPtr _intersection{Unigine::WorldIntersection::create()};
	Unigine::Vector<Unigine::NodePtr> _spawned{};
	int _head{0}; // ring-buffer write index once _spawned is full
	bool _lastGrounded{false};
};
