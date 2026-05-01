#include "MainMenuWorld.h"
#include "utils/Utils.h"
#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineMaterials.h>
#include <UnigineRender.h>
#include <UnigineWorld.h>
#include <cstring>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(MainMenuWorld);

void MainMenuWorld::init()
{
	FLOGERR(start, "start is not set!\n");
	FLOGERR(exit, "exit is not set!\n");
	FLOGERR(strcmp(outlineMat, "") != 0, "outline material is not set!\n");
	FLOGERR(strcmp(startWorld, "") != 0, "start world is not set!\n");
	FLOGERR(_player = Game::getPlayer(), "player not found!\n");

	_outline_material = Materials::findMaterialByPath(outlineMat);
	FLOGERR(_outline_material, "outline material '%s' not found!\n", outlineMat.get());

	_player->addScriptableMaterial(_outline_material);

	Input::setMouseHandle(Input::MOUSE_HANDLE_SOFT);
	Input::setMouseGrab(false);
	Input::setMouseCursorHide(false);

	cache_interactive(start);
	cache_interactive(exit);
}

void MainMenuWorld::update()
{
	ObjectPtr hit = get_mouse_intersection();

	if (hit != _hovered)
	{
		set_highlighted(_hovered, false);
		set_highlighted(hit, true);
		_hovered = hit;
	}

	if (_hovered && Input::isMouseButtonDown(Input::MOUSE_BUTTON_LEFT))
	{
		if (start && _hovered->getID() == start->getID())
			on_start();
		else if (exit && _hovered->getID() == exit->getID())
			on_exit();
	}
}

void MainMenuWorld::on_start()
{
	World::loadWorld(startWorld);
}

void MainMenuWorld::on_exit()
{
	Engine::get()->quit();
}

ObjectPtr MainMenuWorld::get_mouse_intersection()
{
	ivec2 mouse = Input::getMousePosition();
	dvec3 p0 = _player->getWorldPosition();
	dvec3 p1 = p0 + dvec3(_player->getDirectionFromMainWindow(mouse.x, mouse.y)) * 100;

	WorldIntersectionPtr intersection = WorldIntersection::create();
	return World::getIntersection(p0, p1, intersectionMask, intersection);
}

void MainMenuWorld::cache_interactive(const NodePtr &node)
{
	auto obj = checked_ptr_cast<Object>(node);
	FLOGERR(obj, "interactive node '%s' is not an Object\n", node->getName());
	FLOGERR(obj->getNumSurfaces() > 0, "interactive object '%s' has no surfaces\n", node->getName());

	Interactive entry;
	entry.obj = obj;
	entry.mat = obj->getMaterialInherit(0);
	entry.aux_state_idx = entry.mat->findState("auxiliary");
	FLOGERR(entry.aux_state_idx >= 0, "material on '%s' has no 'auxiliary' state\n", node->getName());

	entry.mat->setState(entry.aux_state_idx, 0);
	_interactives.append(entry);
}

void MainMenuWorld::set_highlighted(const ObjectPtr &obj, bool on)
{
	if (!obj)
		return;

	for (const auto &it : _interactives)
	{
		if (it.obj == obj)
		{
			it.mat->setState(it.aux_state_idx, on ? 1 : 0);
			return;
		}
	}
}
