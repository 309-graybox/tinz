#pragma once
#include "utils/PropertyParameter.h"
#include <UnigineComponentSystem.h>

// Spawns per-surface footstep feedback (mark + sound) when this node touches
// the ground. Goes on a foot node. The surface under the foot is resolved via
// the Surface tag + SurfaceRegistry; marks are kept in a ring buffer capped at
// max_count.
class FootstepFX: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(FootstepFX, Unigine::ComponentBase)
	COMPONENT_UPDATE(update)

	PROPERTY(Mask, ground_mask, ~0)
	PROPERTY(Float, height, 0.2f, Tooltip("Длина луча вниз для поиска земли (м)"))
	PROPERTY(Int, max_count, 10, Tooltip("Сколько следов держать живыми (кольцевой буфер); 0 = без следов"))
	PROPERTY(String, default_surface, "default", Tooltip("Ключ поверхности, если под ногой нет компонента Surface"))

private:
	void update();

private:
	Unigine::WorldIntersectionPtr _intersection{Unigine::WorldIntersection::create()};
	Unigine::Vector<Unigine::NodePtr> _spawned{};
	int _head{0}; // ring-buffer write index once _spawned is full
	bool _lastGrounded{false};
};
