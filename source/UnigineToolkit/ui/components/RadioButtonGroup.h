#pragma once

#include "../elements/CheckBox.h"
#include "../elements/Element.h"

#include <UnigineComponentSystem.h>

namespace UI {

class RadioButtonGroup : public Unigine::ComponentBase
{
public:
	COMPONENT(RadioButtonGroup, ComponentBase);
	PROP_NAME("UIC_RadioButtonGroup");
	COMPONENT_INIT(init, -990);

	PROP_ARRAY(Node, checkbox_nodes);

	// interaction
	bool selectCheckBox(CheckBox *checkbox);
	bool selectCheckBox(int index);
	int getSelectedCheckBoxIndex() const;
	CheckBox *getSelectedCheckBox() const;

	Unigine::Event<RadioButtonGroup *> &getEventChanged() { return changed_event; }

protected:
	void init();
	void shutdown();

	Unigine::Vector<CheckBoxPtr> checkboxes;
	bool ignore_events = false;

	// events
	Unigine::EventInvoker<RadioButtonGroup *> changed_event;
};
}	 // namespace UI
