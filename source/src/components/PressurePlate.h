#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>
#include <UnigineWorlds.h>

class PressurePlate: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PressurePlate, Unigine::ComponentBase)
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Node, plate, "", "plate to move");
	PROP_PARAM(Node, trigger, "Trigger", "WorldTrigger that detects stepping on spikes")
	PROP_PARAM(Float, depth, 0.1f);
	PROP_PARAM(String, pressSoundId);
	PROP_PARAM(String, unpressSoundId);
	PROP_PARAM(Float, speed, 8.0f, "Speed", "Approach rate toward open pose")
	PROP_PARAM(Float, damping, 8.0f, "Damping", "Return rate to rest pose")

private:
	void init();
	void update();

public: 
	void onEnter(const Unigine::NodePtr &n);
	void onLeave(const Unigine::NodePtr &n);
	bool isPlayerNode(const Unigine::NodePtr &n) const;
	void lock();
	void release();

	Unigine::Event<> &getEventPressed() { return pressed_event; };
	Unigine::Event<> &getEventUnpressed() { return unpressed_event; };

	bool _need_update = false;
	bool _player_inside = false;
	bool _press = false;
	bool _locked = false;
	float _current = 0.0f;
	
	Unigine::WorldTriggerPtr _trigger;
	Unigine::Math::dvec3 _default_pos;

	Unigine::EventInvoker<> pressed_event;
	Unigine::EventInvoker<> unpressed_event;
};