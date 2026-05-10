#include "components/interaction/Inventory.h"

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

int Inventory::removeItem(const char *type_id, int amount)
{
	if (!type_id || !*type_id || amount <= 0)
		return 0;

	String key(type_id);
	const int current = _items.value(key);
	if (current <= 0)
		return 0;

	const int removed = min(current, amount);
	const int next = current - removed;

	if (next > 0)
		_items.append(key, next);
	else
		_items.remove(key);

	Log::message("Inventory: %s -%d = %d\n", type_id, removed, max(next, 0));
	_event_item_changed.run(key.get(), max(next, 0));
	return removed;
}

int Inventory::getCount(const char *type_id) const
{
	if (!type_id || !*type_id)
		return 0;

	return _items.value(String(type_id));
}
