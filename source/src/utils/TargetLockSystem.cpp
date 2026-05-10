#include "TargetLockSystem.h"
#include <UnigineVisualizer.h>
#include <components/enemies/Targetable.h>

using namespace Unigine;
using namespace Unigine::Math;

Vector<Targetable *> TargetFinder::filter(Vector<Targetable *> targets, const FinderConfig &conf)
{
	for (auto it = targets.begin(); it != targets.end();)
	{
		if (it.get() && it.get()->getNode() && isInside(it.get()->getNode()->getWorldPosition(), conf))
		{
			++it;
		} else
		{
			it = targets.erase(it);
		}
	}

	return targets;
}

Vector<Targetable *> TargetFinder::filterTargetableOnly(const Vector<NodePtr> &nodes)
{
	auto cs = ComponentSystem::get();
	Vector<Targetable *> targets;
	for (const auto &node : nodes)
	{
		auto targetable = cs->getComponent<Targetable>(node);
		if (targetable)
			targets.append(targetable);
	}
	return targets;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool TargetFinderBoundSphere::isInside(const Vec3 &pos, const FinderConfig &conf) const
{
	return WorldBoundSphere(conf.player->getWorldPosition(), conf.dist).inside(pos);
}

Vector<Targetable *> TargetFinderBoundSphere::scan(const FinderConfig &conf) const
{
	Vector<NodePtr> nodes;
	World::getIntersection(WorldBoundSphere(conf.player->getWorldPosition(), conf.dist), nodes);
	return filterTargetableOnly(nodes);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool TargetFinderBoundFrustum::isInside(const Vec3 &pos, const FinderConfig &conf) const
{
	auto camera = conf.player->getCamera();
	if (!camera)
		return false;
	auto oldZFar = camera->getZFar();
	camera->setZFar(conf.dist);
	WorldBoundFrustum frustum(camera->getProjection(), camera->getModelview());
	camera->setZFar(oldZFar);

	return frustum.inside(pos);
}

Vector<Targetable *> TargetFinderBoundFrustum::scan(const FinderConfig &conf) const
{
	auto camera = conf.player->getCamera();
	if (!camera)
		return {};
	auto oldZFar = camera->getZFar();
	camera->setZFar(conf.dist);
	WorldBoundFrustum frustum(camera->getProjection(), camera->getModelview());
	camera->setZFar(oldZFar);

	Vector<NodePtr> nodes;
	World::getIntersection(frustum, nodes);
	return filterTargetableOnly(nodes);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool TargetComparatorNearest::compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const
{
	auto d1 = distance2(player->getWorldPosition(), a->getNode()->getWorldPosition());
	auto d2 = distance2(player->getWorldPosition(), b->getNode()->getWorldPosition());
	return d1 < d2;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool TargetComparatorFurthest::compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const
{
	auto d1 = distance2(player->getWorldPosition(), a->getNode()->getWorldPosition());
	auto d2 = distance2(player->getWorldPosition(), b->getNode()->getWorldPosition());
	return d1 > d2;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool TargetComparatorMedial::compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const
{
	auto pos = player->getWorldPosition();
	auto player_dir = Vec3(player->getWorldDirection());

	auto aPos = a->getNode()->getWorldPosition();
	auto bPos = b->getNode()->getWorldPosition();

	auto aDot = dot(player_dir, normalize(pos - aPos));
	auto bDot = dot(player_dir, normalize(pos - bPos));

	return aDot < bDot;
}
