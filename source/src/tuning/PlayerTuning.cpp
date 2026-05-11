#include "PlayerTuning.h"
#include "player/movement/CharacterMovement.h"
#include <UnigineProperties.h>
#include <cstdio>

REGISTER_COMPONENT(PlayerTuning)

using namespace Unigine;

void PlayerTuning::init()
{
	TuningBase::init();
	if (!_ok)
		return;

	configure();
}

void PlayerTuning::configure()
{
	Vector<CharacterMovement *> movements;
	ComponentSystem::get()->getComponentsInWorld<CharacterMovement>(movements);

	int registered_before = _commands.size();

	for (int i = 0; i < movements.size(); ++i)
	{
		auto *cm = movements[i];

		char buf[32];
		std::snprintf(buf, sizeof(buf), "player[%d].", i);
		String prefix(buf);

		for (const auto &v : cm->variables)
		{
			auto param = v->getParameter();
			switch (param->getType())
			{
				case Property::PARAMETER_TOGGLE:
				case Property::PARAMETER_INT:
				case Property::PARAMETER_FLOAT:
				case Property::PARAMETER_DOUBLE:
				case Property::PARAMETER_STRING:
					break;
				default:
					continue;
			}
			addConsoleCommand(prefix + param->getName(), param);
		}
	}

	Log::message("PlayerTuning: registered %d CharacterMovement(s), %d command(s)\n",
		movements.size(), _commands.size() - registered_before);
}
