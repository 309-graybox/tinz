#pragma once
#include <UnigineComponentSystem.h>

namespace UI
{
class Canvas;
class Table;
class Sprite;
class Label;
} // namespace UI

class Entity;

class HUD: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(HUD, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(File, hpSprite)
	PROP_PARAM(Int, hpDivider, 25)

	PROP_PARAM(String, soulsTypeId, "soul")

public:
	void setBowlSouls(int deposited, int required);
	void hideBowlSouls();

private:
	void init();
	void shutdown();

private:
	void onHpChanged(Entity *ent);
	void onPlayerDied(Entity *ent);
	void onInventoryItemChanged(const char *type_id, int count);
	void onSoulsChanged(int n); // Not int!

	void updateHearts(int hearts);
	void show(bool show);

private:
	UI::Canvas *_canvas{nullptr};
	UI::Table *_tableHp{nullptr};
	UI::Label *_labelSouls{nullptr};
	UI::Sprite *_spriteBowlSouls{nullptr};
	UI::Label *_labelBowlSouls{nullptr};
};
