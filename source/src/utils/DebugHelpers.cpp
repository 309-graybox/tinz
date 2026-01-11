#include "DebugHelpers.h"
#include <UnigineVisualizer.h>
#include <UnigineConsole.h>

using namespace Unigine;

VisualizerHelper::VisualizerHelper()
	: _init(Visualizer::isEnabled())
{
}

VisualizerHelper::~VisualizerHelper()
{
	Visualizer::setEnabled(_init);
}

void VisualizerHelper::setEnabled(bool v)
{
	Visualizer::setEnabled(v);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
ConsoleHelper::ConsoleHelper()
	: _init(Console::isOnscreen())
{
}

ConsoleHelper::~ConsoleHelper()
{
	Console::setOnscreen(_init);
}

void ConsoleHelper::setOnscreen(bool v)
{
	Console::setOnscreen(v);
}
