#pragma once
#include <UnigineComponentSystem.h>

class Entity;

class HUD: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(HUD, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(File, font)

	PROP_PARAM(File, hpIcon)
	PROP_PARAM(IVec2, hpIconSize, {50, 50})
	PROP_PARAM(Int, hpDivider, 25)

	PROP_PARAM(File, soulIcon)
	PROP_PARAM(IVec2, soulIconSize, {50, 50})
	PROP_PARAM(Int, soulIconFontSize, 32)
	PROP_PARAM(String, soulsTypeId, "soul")

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
	Unigine::WidgetHBoxPtr _hpBar;
	Unigine::WidgetHBoxPtr _soulsBar;
	Unigine::WidgetLabelPtr _soulsCount;

	Unigine::ImagePtr _hpImage;
	Unigine::ImagePtr _soulImage;
};
