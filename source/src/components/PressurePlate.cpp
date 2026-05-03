#include "PressurePlate.h"
#include "utils/Utils.h"
#include "game/GameState.h"
#include "utils/Utils.h"
#include <UnigineComponentSystem.h>
#include <UnigineEvent.h>
#include <UnigineGame.h>
#include <UnigineNodes.h>
#include <UnigineObjects.h>
#include <UniginePtr.h>
#include <cstring>

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

	auto ref = checked_ptr_cast<NodeReference>(plate.get());
	if (!ref)
		_node = checked_ptr_cast<ObjectMeshStatic>(plate.get());
	else
		_node = checked_ptr_cast<ObjectMeshStatic>(ref->getReference());
	FLOGERR(_node, "not ObjectMeshStatic");

	for (int i = 0; i < _node->getNumSurfaces(); ++i)
	{
		if (strcmp(_node->getSurfaceName(i), "LightMineral_v2") == 0)
		{
			_mat = _node->getMaterialInherit(i);
			_emission_idx = _mat->findParameter("emission_color");
			FLOGERR(_emission_idx != -1, "no emission");
		}
	}
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

	_mat->setParameterFloat4(_emission_idx, vec4(0.88f, 0.64f, 0.7f, 1.0f));
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

	_mat->setParameterFloat4(_emission_idx, vec4_zero);
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
	if (!_player_inside)
		_mat->setParameterFloat4(_emission_idx, vec4_zero);
	_locked = false;
	_need_update = true;
}