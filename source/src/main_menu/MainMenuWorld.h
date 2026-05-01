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
	COMPONENT_SHUTDOWN(shutdown);

	PROP_PARAM(Node, start);
	PROP_PARAM(Node, exit);
	PROP_PARAM(File, startWorld);
	PROP_PARAM(File, outlineMat);
	PROP_PARAM(String, backgroundMusic);
	PROP_PARAM(Mask, intersectionMask, ~0);

private:
	void init();
	void update();
	void shutdown();

	struct SurfaceMat
	{
		Unigine::MaterialPtr mat;
		int aux_state_idx = -1;
	};

	struct Interactive
	{
		Unigine::NodePtr root;
		Unigine::Vector<SurfaceMat> surfaces;
	};

	void on_start();
	void on_exit();

	Unigine::ObjectPtr get_mouse_intersection();

	void cache_interactive(const Unigine::NodePtr &node);
	void collect_surfaces(const Unigine::NodePtr &node, Unigine::Vector<SurfaceMat> &out);
	Interactive *find_interactive_for(const Unigine::ObjectPtr &obj);
	void set_highlighted(Interactive *it, bool on);

	Unigine::PlayerPtr _player;
	Unigine::MaterialPtr _outline_material;
	Unigine::Vector<Interactive> _interactives;
	Interactive *_hovered = nullptr;

	Unigine::Input::MOUSE_HANDLE _mouse_handle;
	// bool _mouse_grab = false;
	// bool _mouse_cursor_hide = false;
};

