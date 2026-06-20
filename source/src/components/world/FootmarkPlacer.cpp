#include "FootmarkPlacer.h"
#include <UnigineGame.h>

REGISTER_COMPONENT(FootmarkPlacer)

using namespace Unigine;
using namespace Unigine::Math;

void FootmarkPlacer::update()
{
	auto p = node->getWorldPosition();

	auto obj = World::getIntersection(p, p + Vec3_down * height, ground_mask, _intersection);
	if (obj)
	{
		if (_lastGrounded)
			return;

		_lastGrounded = true;

		auto n = World::loadNode(node_to_place);
		COMPONENT_REQUIRE(n, "[FootmarkPlacer::update]: Can not find node to spawn\n", return);

		auto ip = _intersection->getPoint();
		auto nr = node->getWorldRotation();
		auto au = nr.getAngle(vec3_up);

		n->setWorldPosition(ip + Vec3(offset));
		n->setWorldRotation(quat(rotate(vec3_up, au)));

		if (_spawned.size() < max_count)
		{
			_spawned.append(n);
		} else
		{
			_spawned[_head].deleteLater();
			_spawned[_head] = n;
			_head = (_head + 1) % max_count;
		}
	} else
	{
		_lastGrounded = false;
	}
}
