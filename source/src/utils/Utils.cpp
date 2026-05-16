#include "Utils.h"
#include <UnigineConsole.h>

using namespace Unigine;

void addConsoleCommand(const Unigine::String &name, const Unigine::PropertyParameterPtr &param)
{
	if (Console::isCommand(name))
		Console::removeCommand(name);

	Console::addCommand(name, param->getTooltip(), MakeCallback([param](int argc, char **argv) {
		if (argc == 1)
		{
			switch (param->getType())
			{
				case Property::PARAMETER_TOGGLE:
					Log::message("%s %i\n", argv[0], param->getValueToggle());
					break;
				case Property::PARAMETER_INT:
					Log::message("%s %i\n", argv[0], param->getValueInt());
					break;
				case Property::PARAMETER_FLOAT:
					Log::message("%s %f\n", argv[0], param->getValueFloat());
					break;
				case Property::PARAMETER_DOUBLE:
					Log::message("%s %f\n", argv[0], param->getValueDouble());
					break;
				case Property::PARAMETER_STRING:
					Log::message("%s %s\n", argv[0], param->getValueString());
					break;
				default:
					Log::error("Unsupported parameter type for command: %s\n", argv[0]);
					break;
			}
			return;
		}

		if (argc != 2)
		{
			Log::error("Invalid command argc: %i, required %i\n", argc, 2);
			return;
		}

		switch (param->getType())
		{
			case Property::PARAMETER_TOGGLE:
				param->setValueToggle(String::atoi(argv[1]) != 0);
				break;
			case Property::PARAMETER_INT:
				param->setValueInt(String::atoi(argv[1]));
				break;
			case Property::PARAMETER_FLOAT:
				param->setValueFloat(String::atof(argv[1]));
				break;
			case Property::PARAMETER_DOUBLE:
				param->setValueDouble(String::atod(argv[1]));
				break;
			case Property::PARAMETER_STRING:
				param->setValueString(argv[1]);
				break;
			default:
				Log::error("Unsupported parameter type for command: %s\n", param->getName());
				break;
		}
	}));
}

void removeConsoleCommand(const Unigine::String &name)
{
	Console::removeCommand(name);
}
