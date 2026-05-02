#include "HUD.h"
#include "utils/Utils.h"
#include "components/Entity.h"
#include "components/Inventory.h"
#include "game/GameState.h"

#include <cstring>

REGISTER_COMPONENT(HUD)

using namespace Unigine;
using namespace Unigine::Math;

void HUD::init()
{
	auto player = game::GameState::getPlayerCharacter();
	FLOGERR(player, "HUD can only work with player\n");

	auto ent = getComponent<Entity>(player);
	FLOGERR(ent, "HUD can only work with entity\n");

	auto gui = Gui::getCurrent();
	FLOGERR(gui, "HUD can only work with gui\n");

	_hpBar = WidgetHBox::create(gui);
	gui->addChild(_hpBar, Gui::ALIGN_OVERLAP | Gui::ALIGN_LEFT | Gui::ALIGN_TOP);

	_hpImage = Image::create(hpIcon);
	FLOGERR(_hpImage, "can't load hp image\n");

	bool r = _hpImage->resize(hpIconSize.get().x, hpIconSize.get().y);
	FLOGERR(r, "can't resize hp image\n");

	_soulsBar = WidgetHBox::create(gui);
	gui->addChild(_soulsBar, Gui::ALIGN_OVERLAP | Gui::ALIGN_RIGHT | Gui::ALIGN_BOTTOM);

	_soulsCount = WidgetLabel::create(gui, "0");
	_soulsCount->setFont(font);
	_soulsCount->setFontSize(soulIconFontSize);
	_soulsBar->addChild(_soulsCount);

	_soulImage = Image::create(soulIcon);
	FLOGERR(_soulImage, "can't load souls image\n");

	const auto soul_size = soulIconSize.get();
	r = _soulImage->resize(soul_size.x, soul_size.y);
	FLOGERR(r, "can't resize souls image\n");

	auto soulsIcon = WidgetSprite::create(gui);
	soulsIcon->setImage(_soulImage);
	soulsIcon->setWidth(soul_size.x);
	soulsIcon->setHeight(soul_size.y);
	_soulsBar->addChild(soulsIcon);

	ent->eventDied().connect(this, &HUD::onPlayerDied);
	ent->hpChanged().connect(this, &HUD::onHpChanged);
	onHpChanged(ent);

	if (auto inventory = getComponent<Inventory>(player))
	{
		inventory->itemChanged().connect(this, &HUD::onInventoryItemChanged);
		onSoulsChanged(inventory->getCount(soulsTypeId.get()));
	}
}

void HUD::shutdown()
{
	_hpBar.deleteLater();
	_soulsBar.deleteLater();
	_hpImage.clear();
	_soulImage.clear();
}

void HUD::onHpChanged(Entity *ent)
{
	if (!ent)
		return;

	show(!ent->isDead());

	const float hp = max(ent->getHP(), 0.0f);
	const int divider = max(hpDivider.get(), 1);
	const int hearts = max(ceilInt(hp / (float)divider), 0);
	updateHearts(hearts);
}

void HUD::onPlayerDied(Entity *ent)
{
	UNIGINE_UNUSED(ent)
	show(false);
}

void HUD::onInventoryItemChanged(const char *type_id, int count)
{
	if (!type_id || strcmp(type_id, soulsTypeId.get()) != 0)
		return;

	onSoulsChanged(count);
}

void HUD::onSoulsChanged(int n)
{
	_soulsCount->setText(String::itoa(max(n, 0)));
}

void HUD::show(bool show)
{
	_hpBar->setHidden(!show);
	_soulsBar->setHidden(!show);
}

void HUD::updateHearts(int hearts)
{
	if (!_hpBar)
		return;

	const auto size = hpIconSize.get();
	int current = _hpBar->getNumChildren();

	while (current < hearts)
	{
		auto heart = WidgetSprite::create(Gui::getCurrent());
		heart->setImage(_hpImage);
		heart->setWidth(size.x);
		heart->setHeight(size.y);
		_hpBar->addChild(heart);
		++current;
	}

	while (current > hearts)
	{
		auto heart = _hpBar->getChild(current - 1);
		_hpBar->removeChild(heart);
		heart.deleteLater();
		--current;
	}
}
