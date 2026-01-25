#include "Targetable.h"

REGISTER_COMPONENT(Targetable)

using namespace Unigine;
using namespace Unigine::Math;

NodePtr Targetable::getTarget(const Unigine::Math::Vec3& from)
{
	// TODO
	// There will be some very smart in feature
	return targets.size() == 0 ? node : targets.get(randInt(0, targets.size()));
}
