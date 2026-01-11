#pragma once

class VisualizerHelper
{
public:
	VisualizerHelper();
	~VisualizerHelper();

	void setEnabled(bool v);

private:
	bool _init;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class ConsoleHelper
{
public:
	ConsoleHelper();
	~ConsoleHelper();

	void setOnscreen(bool v);

private:
	bool _init;
};
