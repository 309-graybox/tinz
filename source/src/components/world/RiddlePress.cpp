#include "components/world/RiddlePress.h"
#include <UnigineComponentSystem.h>
#include "components/world/PressurePlate.h"
#include "components/fx/AffineModifier.h"

#include "utils/Utils.h"

REGISTER_COMPONENT(RiddlePress);

using namespace Unigine;
using namespace Unigine::Math;

void RiddlePress::init()
{
	_plates.reserve(plates.size());
	for (int i = 0; i < plates.size(); ++i)
	{
		FLOGERR(plates[i], "NOo node");

		auto comp = getComponent<PressurePlate>(plates[i]);
		FLOGERR(comp, "NOo comp");

		_plates.append(comp);

		comp->getEventPressed().connect(_conn, [this, i]() {
			RiddlePress::pressed(i);
		});
	}

	FLOGERR(door, "No door");
	_door = getComponent<AffineModifier>(door);
	FLOGERR(_door, "Door without AffineMod");
}

void RiddlePress::shutdown()
{
	_conn.disconnectAll();
}

void RiddlePress::pressed(int pressed)
{
	if (_next != pressed)
	{
		release();
		return;
	}

	++_next;
	_plates[pressed]->lock();

	if (pressed == _plates.size() - 1)
	{
		win();
		for (int i = 0; i < plates.size(); ++i)
		{
			removeComponent<PressurePlate>(plates[i]);
			removeComponent<RiddlePress>(node);
		}
	}
}

void RiddlePress::release()
{
	_next = 0;
	for (auto comp : _plates)
	{
		comp->release();
	}
}

void RiddlePress::win()
{
	Log::message("WIN\n");
	_door->setOpen(true);
}