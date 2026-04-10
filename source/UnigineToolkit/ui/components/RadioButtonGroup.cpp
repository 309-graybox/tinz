#include "RadioButtonGroup.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(RadioButtonGroup);

void RadioButtonGroup::init()
{
	ignore_events = false;
	checkboxes.clear();
	for (int i = 0; i < checkbox_nodes.size(); i++)
	{
		CheckBoxPtr ch = CheckBoxPtr(getComponent<CheckBox>(checkbox_nodes[i].get()));
		if (ch)
		{
			ch->getEventCheckBoxChanged().connect(*this, [this](CheckBox *c) {
				if (ignore_events)
					return;
				if (!c->isChecked())
				{
					// user can't uncheck radiobutton
					c->setChecked(true);
				}
				else
				{
					// uncheck all other checkboxes
					ignore_events = true;
					for (int i = 0; i < checkboxes.size(); i++)
					{
						const CheckBoxPtr &ch = checkboxes[i];
						if (ch && ch->isChecked() && ch != c)
							ch->setChecked(false);
					}
					ignore_events = false;
				}
			});
		}
		checkboxes.append(ch);
	}
}

bool RadioButtonGroup::selectCheckBox(CheckBox *checkbox)
{
	for (int i = 0; i < checkboxes.size(); i++)
	{
		if (checkbox == checkboxes[i])
		{
			if (!checkbox->isChecked())
			{
				checkbox->setChecked(true);
				changed_event.run(this);
			}
			return true;
		}
	}
	return false;
}

bool RadioButtonGroup::selectCheckBox(int index)
{
	if (index < 0 || index >= checkboxes.size())
		return false;

	const CheckBoxPtr &ch = checkboxes[index];
	if (ch)
	{
		if (!ch->isChecked())
		{
			ch->setChecked(true);
			changed_event.run(this);
		}
		return true;
	}

	return false;
}

int RadioButtonGroup::getSelectedCheckBoxIndex() const
{
	for (int i = 0; i < checkboxes.size(); i++)
	{
		const CheckBoxPtr &ch = checkboxes[i];
		if (ch && ch->isChecked())
			return i;
	}
	return -1;
}

CheckBox *RadioButtonGroup::getSelectedCheckBox() const
{
	for (int i = 0; i < checkboxes.size(); i++)
	{
		const CheckBoxPtr &ch = checkboxes[i];
		if (ch && ch->isChecked())
			return ch.get();
	}
	return nullptr;
}
