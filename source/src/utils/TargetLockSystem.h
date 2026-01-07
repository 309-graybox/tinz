#pragma once
#include <UnigineMathLib.h>
#include <UnigineNode.h>
#include <UnigineWorld.h>
#include <UniginePlayers.h>
#include <memory>

class Targetable;

struct FinderConfig
{
	Unigine::PlayerPtr player;
	Unigine::Math::Scalar dist;
};

class TargetFinder
{
public:
	virtual ~TargetFinder() = default;

	virtual bool isInside(const Unigine::Math::Vec3 &pos, const FinderConfig &conf) const = 0;

	virtual Unigine::Vector<Targetable *> scan(const FinderConfig &conf) const = 0;
	virtual Unigine::Vector<Targetable *> filter(Unigine::Vector<Targetable *> targets, const FinderConfig &conf);

protected:
	static Unigine::Vector<Targetable *> filterTargetableOnly(const Unigine::Vector<Unigine::NodePtr> &nodes);
};
using TargetFinderPtr = std::unique_ptr<TargetFinder>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class TargetFinderBoundSphere: public TargetFinder
{
public:
	bool isInside(const Unigine::Math::Vec3 &pos, const FinderConfig &conf) const override;

	Unigine::Vector<Targetable *> scan(const FinderConfig &conf) const override;

	static std::unique_ptr<TargetFinderBoundSphere> create() { return std::make_unique<TargetFinderBoundSphere>(); }
};
using TargetFinderBoundSpherePtr = std::unique_ptr<TargetFinderBoundSphere>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class TargetFinderBoundFrustum: public TargetFinder
{
public:
	bool isInside(const Unigine::Math::Vec3 &pos, const FinderConfig &conf) const override;

	Unigine::Vector<Targetable *> scan(const FinderConfig &conf) const override;

	static std::unique_ptr<TargetFinderBoundFrustum> create() { return std::make_unique<TargetFinderBoundFrustum>(); }
};
using TargetFinderBoundFrustumPtr = std::unique_ptr<TargetFinderBoundFrustum>;


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class TargetComparator
{
public:
	virtual ~TargetComparator() = default;
	virtual bool compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const = 0;
};
using TargetComparatorPtr = std::unique_ptr<TargetComparator>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class TargetComparatorNearest: public TargetComparator
{
public:
	bool compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const override;

	static std::unique_ptr<TargetComparatorNearest> create() { return std::make_unique<TargetComparatorNearest>(); }
};
using TargetComparatorNearestPtr = std::unique_ptr<TargetComparatorNearest>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class TargetComparatorFurthest: public TargetComparator
{
public:
	bool compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const override;

	static std::unique_ptr<TargetComparatorFurthest> create() { return std::make_unique<TargetComparatorFurthest>(); }
};
using TargetComparatorFurthestPtr = std::unique_ptr<TargetComparatorFurthest>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class TargetComparatorMedial: public TargetComparator
{
public:
	bool compare(const Unigine::PlayerPtr &player, Targetable *a, Targetable *b) const override;

	static std::unique_ptr<TargetComparatorMedial> create() { return std::make_unique<TargetComparatorMedial>(); }
};
using TargetComparatorMedialPtr = std::unique_ptr<TargetComparatorMedial>;

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
UNIGINE_INLINE void sort(Unigine::Vector<Targetable *> &targets, const Unigine::PlayerPtr &player, TargetComparator *pred)
{
	std::sort(targets.begin(), targets.end(), [player, pred](Targetable *a, Targetable *b)
	{
		return pred->compare(player, a, b);
	});
}
