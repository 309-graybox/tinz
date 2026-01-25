#pragma once
#include <components/Entity.h>

class Targetable: public Entity
{
public:
	COMPONENT_DEFINE(Targetable, Entity)

	PROP_ARRAY(Node, targets)

	Unigine::NodePtr getTarget(const Unigine::Math::Vec3& from);
};
