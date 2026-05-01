#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineMaterial.h>
#include <UnigineObjects.h>
#include <UniginePlayers.h>
#include <UnigineVector.h>


class MainMenuWorld : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(MainMenuWorld, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);

	PROP_PARAM(Node, start);
	PROP_PARAM(Node, exit);
	PROP_PARAM(File, startWorld);
	PROP_PARAM(File, outlineMat);
	PROP_PARAM(Mask, intersectionMask, ~0);

private:
	struct Interactive
	{
		Unigine::ObjectPtr obj;
		Unigine::MaterialPtr mat;
		int aux_state_idx = -1;
	};

	void init();
	void update();

	void on_start();
	void on_exit();

	Unigine::ObjectPtr get_mouse_intersection();
	void cache_interactive(const Unigine::NodePtr &node);
	void set_highlighted(const Unigine::ObjectPtr &obj, bool on);

	Unigine::PlayerPtr _player;
	Unigine::MaterialPtr _outline_material;
	Unigine::Vector<Interactive> _interactives;
	Unigine::ObjectPtr _hovered;
};

