#include "MainMenuWorld.h"
#include "utils/Utils.h"
#include <UnigineEngine.h>
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

	_interactives.reserve(2);
	cache_interactive(start);
	cache_interactive(exit);
}

void MainMenuWorld::update()
{
	ObjectPtr hit = get_mouse_intersection();
	Interactive *hit_root = find_interactive_for(hit);

	if (hit_root != _hovered)
	{
		set_highlighted(_hovered, false);
		set_highlighted(hit_root, true);
		_hovered = hit_root;
	}

	if (_hovered && Input::isMouseButtonDown(Input::MOUSE_BUTTON_LEFT))
	{
		if (start && _hovered->root->getID() == start->getID())
			on_start();
		else if (exit && _hovered->root->getID() == exit->getID())
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
	Interactive entry;
	entry.root = node;
	collect_surfaces(node, entry.surfaces);
	FLOGERR(entry.surfaces.size() > 0,
		"interactive '%s' has no Object descendants with 'auxiliary' state\n", node->getName());

	for (auto &sm : entry.surfaces)
		sm.mat->setState(sm.aux_state_idx, 0);

	_interactives.append(entry);
}

void MainMenuWorld::collect_surfaces(const NodePtr &node, Vector<SurfaceMat> &out)
{
	if (auto obj = checked_ptr_cast<Object>(node))
	{
		for (int s = 0; s < obj->getNumSurfaces(); ++s)
		{
			auto mat = obj->getMaterialInherit(s);
			if (!mat)
				continue;
			int idx = mat->findState("auxiliary");
			if (idx < 0)
				continue;
			out.append({mat, idx});
		}
	}
	for (int i = 0; i < node->getNumChildren(); ++i)
		collect_surfaces(node->getChild(i), out);
}

MainMenuWorld::Interactive *MainMenuWorld::find_interactive_for(const ObjectPtr &obj)
{
	if (!obj)
		return nullptr;
	NodePtr cur = obj;
	while (cur)
	{
		for (auto &it : _interactives)
		{
			if (it.root && it.root->getID() == cur->getID())
				return &it;
		}
		cur = cur->getParent();
	}
	return nullptr;
}

void MainMenuWorld::set_highlighted(Interactive *it, bool on)
{
	if (!it)
		return;
	const int v = on ? 1 : 0;
	for (auto &sm : it->surfaces)
		sm.mat->setState(sm.aux_state_idx, v);
}
