#include "Inventory.h"

#include <UnigineLog.h>

REGISTER_COMPONENT(Inventory)

using namespace Unigine;
using namespace Unigine::Math;

int Inventory::addItem(const char *type_id, int amount)
{
	if (!type_id || !*type_id)
		return 0;
	if (amount <= 0)
		return getCount(type_id);

	String key(type_id);
	const int count = _items.value(key) + amount;
	_items.append(key, count);

	Log::message("Inventory: %s +%d = %d\n", type_id, amount, count);
	_event_item_changed.run(key.get(), count);
	return count;
}

int Inventory::getCount(const char *type_id) const
{
	if (!type_id || !*type_id)
		return 0;

	return _items.value(String(type_id));
}
