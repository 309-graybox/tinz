#include <UnigineCallback.h>
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>
#include <UnigineWorlds.h>

class PressurePlate;
class AffineModifier;

class RiddlePress : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(RiddlePress, Unigine::ComponentBase)
	COMPONENT_INIT(init);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_ARRAY(Node, plates);
	PROP_PARAM(Node, door);

private:
	void init();
	void shutdown();

	void pressed(int pressed);
	void release();
	void win();
	// void lose(int max);

private:
	Unigine::Vector<PressurePlate*> _plates;
	Unigine::EventConnections _conn;
	AffineModifier* _door{nullptr};

	int _next = 0;
};