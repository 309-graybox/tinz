#include "FootstepFX.h"

#include "Surface.h"
#include "SurfaceRegistry.h"
#include "audio/SoundManager.h"

#include <UnigineGame.h>

REGISTER_COMPONENT(FootstepFX)

using namespace Unigine;
using namespace Unigine::Math;

void FootstepFX::update()
{
	auto p = node->getWorldPosition();
	auto obj = World::getIntersection(p, p + Vec3_down * height, ground_mask, _intersection);
	if (!obj)
	{
		_lastGrounded = false;
		return;
	}

	if (_lastGrounded)
		return;

	_lastGrounded = true;

	Surface *s = getComponentInParent<Surface>(obj);
	const char *type = s ? s->getType() : default_surface.get();
	SurfaceConfig *cfg = SurfaceRegistry::find(type);
	if (!cfg)
		cfg = SurfaceRegistry::find(default_surface.get());

	if (!cfg)
		return;

	const auto ip = _intersection->getPoint();

	const char *sfx = cfg->step_sound.get();
	if (sfx && *sfx)
		audio::SoundManager::play3DAt(sfx, ip);

	const char *mark = cfg->footmark_node.get();
	if (!mark || !*mark)
		return;

	auto n = World::loadNode(mark);
	COMPONENT_REQUIRE(n, "[FootstepFX::update]: can not load footmark node\n", return);

	auto au = node->getWorldRotation().getAngle(vec3_up);
	n->setWorldPosition(ip + Vec3(cfg->offset));
	n->setWorldRotation(quat(rotate(vec3_up, au)));

	if (max_count <= 0)
	{
		n.deleteLater();
		return;
	}

	if (_spawned.size() < max_count)
	{
		_spawned.append(n);
	} else
	{
		_spawned[_head].deleteLater();
		_spawned[_head] = n;
		_head = (_head + 1) % max_count;
	}
}
