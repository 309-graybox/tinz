#include "PressurePlate.h"
#include "utils/Utils.h"
#include "game/GameState.h"
#include "utils/Utils.h"
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>
#include <UnigineGame.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(PressurePlate);

void PressurePlate::init()
{
	FLOGERR(plate, "plate is not set");

	_trigger = checked_ptr_cast<WorldTrigger>(trigger.get());
	FLOGERR(_trigger, "trigger should be WorldTrigger\n");

	_trigger->setTriggerInteractionEnabled(true);
	_trigger->getEventEnter().connect(this, &PressurePlate::onEnter);
	_trigger->getEventLeave().connect(this, &PressurePlate::onLeave);

	_default_pos = plate->getWorldPosition();
}

void PressurePlate::update()
{
	if (!_need_update)
		return;

	const float ifps = Game::getIFps();

	bool press = _player_inside || _locked;
	const float aim = press ? 1.0f : 0.0f;
	const float rate = press ? speed : damping;
	_current = lerp(_current, aim, saturate(rate * ifps));

	const dvec3 current_offset = dvec3(0.0f, 0.0f, -depth) * _current;
	const dvec3 new_pos = _default_pos + current_offset;

	plate->setWorldPosition(new_pos);
}

void PressurePlate::onEnter(const NodePtr &n)
{
	if (!isPlayerNode(n))
		return;

	_player_inside = true;

	if (_locked)
		return;

	_need_update = true;
	pressed_event.run();
}

void PressurePlate::onLeave(const NodePtr &n)
{
	if (!isPlayerNode(n))
		return;
	
	_player_inside = false;

	if (_locked)
		return;

	_need_update = true;
	unpressed_event.run();
}

bool PressurePlate::isPlayerNode(const NodePtr &n) const
{
	NodePtr player = game::GameState::getPlayerCharacter();
	return player && isInHierarchy(n, player);
}

void PressurePlate::lock()
{
	_locked = true;
	_need_update = true;
}

void PressurePlate::release()
{
	_locked = false;
	_need_update = true;
}