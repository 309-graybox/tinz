#pragma once
#include <UnigineComponentSystem.h>

class Targetable: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Targetable, Unigine::ComponentBase)

	PROP_ARRAY(Node, targets)

	Unigine::NodePtr getTarget();
};
