#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>
#include <UnigineHashMap.h>

class Inventory: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(Inventory, Unigine::ComponentBase)

	int addItem(const char *type_id, int amount);
	int removeItem(const char *type_id, int amount); // returns actually removed amount
	int getCount(const char *type_id) const;

	Unigine::EventInvoker<const char *, int> &itemChanged() noexcept { return _event_item_changed; }

private:
	Unigine::HashMap<Unigine::String, int> _items;
	Unigine::EventInvoker<const char *, int> _event_item_changed;
};
