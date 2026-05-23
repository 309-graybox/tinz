#include "ui/HUD.h"
#include "utils/Utils.h"
#include "utils/ToolkitUtils.h"
#include "components/Entity.h"
#include "components/interaction/Inventory.h"
#include "game/GameState.h"

#include "UnigineToolkit/ui/elements/Canvas.h"
#include "UnigineToolkit/ui/elements/Table.h"
#include "UnigineToolkit/ui/elements/Label.h"
#include "UnigineToolkit/ui/elements/Sprite.h"

REGISTER_COMPONENT(HUD)

using namespace Unigine;
using namespace Unigine::Math;

void HUD::init()
{
	auto player = game::GameState::getPlayerCharacter();
	FLOGERR(player, "HUD can only work with player\n");

	auto ent = getComponent<Entity>(player);
	FLOGERR(ent, "HUD can only work with entity\n");

	GET_CANVAS(_canvas, node);
	FLOGERR(_canvas, "Expeced canvas\n");

	_labelSouls = GET_LABEL(_canvas, LabelSouls);
	FLOGERR(_labelSouls, "Expected souls counter\n");

	_spriteBowlSouls = GET_SPRITE(_canvas, SpriteBowlSouls);
	FLOGERR(_spriteBowlSouls, "Expected bowl souls sprite\n");
	_spriteBowlSouls->setEnabled(false);

	_labelBowlSouls = GET_LABEL(_canvas, LabelBowlSouls);
	FLOGERR(_labelSouls, "Expected bowl souls counter\n");
	_labelBowlSouls->setEnabled(false);

	_tableHp = GET_TABLE(_canvas, TableHP);
	FLOGERR(_tableHp, "Expected table hp\n");

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
	_labelSouls->setText(String::itoa(max(n, 0)));
}

void HUD::setBowlSouls(int deposited, int required)
{
	if (!_labelBowlSouls)
		return;

	const int shown = clamp(deposited, 0, max(required, 0));
	_labelBowlSouls->setText(String::format("%d/%d", shown, max(required, 0)).get());
	_spriteBowlSouls->setEnabled(true);
	_labelBowlSouls->setEnabled(true);
}

void HUD::hideBowlSouls()
{
	_spriteBowlSouls->setEnabled(false);
	_labelBowlSouls->setEnabled(false);
}

void HUD::show(bool show)
{
	_canvas->setEnabled(show);
}

void HUD::updateHearts(int hearts)
{
	if (!_tableHp)
		return;

	int current = _tableHp->getNumChildren();

	while (current < hearts)
	{
		NodeDummyPtr sprite_node = NodeDummy::create();
		sprite_node->setParent(node);
		sprite_node->setShowInEditorEnabled(false);
		sprite_node->setSaveToWorldEnabled(false);

		auto sprite = addComponent<UI::Sprite>(sprite_node);
		sprite->initializeElement();
		sprite->setTexture(hpSprite);
		_tableHp->addChild(sprite);
		++current;
	}

	while (current > hearts)
	{
		auto sprite = _tableHp->getChild(current - 1);
		_tableHp->removeChild(sprite);
		sprite->getNode().deleteLater();
		--current;
	}
}
