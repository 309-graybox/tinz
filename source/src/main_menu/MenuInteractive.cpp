#include "MenuInteractive.h"
#include "audio/SoundManager.h"

#include <UnigineObjects.h>

REGISTER_COMPONENT(MenuInteractive);

using namespace Unigine;
using namespace Math;

void MenuInteractive::init()
{
	_rest_pos = node->getPosition();
	collect_surfaces(node);
	setOutlineEnabled(false);
	onInit();
}

void MenuInteractive::update()
{
	onUpdate();
}

void MenuInteractive::setHovered(bool on, bool play_sound)
{
	if (_hovered == on)
		return;
	_hovered = on;
	setOutlineEnabled(on);

	if (!play_sound)
		return;
	const char *sfx = on ? hoverEnterSound.get() : hoverExitSound.get();
	if (sfx && *sfx)
		audio::SoundManager::play2D(sfx);
}

void MenuInteractive::setOutlineEnabled(bool on)
{
	const int v = on ? 1 : 0;
	for (auto &sm : _surfaces)
		sm.mat->setState(sm.aux_state_idx, v);
}

void MenuInteractive::collect_surfaces(const NodePtr &n)
{
	if (auto obj = checked_ptr_cast<Object>(n))
	{
		for (int s = 0; s < obj->getNumSurfaces(); ++s)
		{
			auto mat = obj->getMaterialInherit(s);
			if (!mat)
				continue;
			int idx = mat->findState("auxiliary");
			if (idx < 0)
				continue;
			_surfaces.append({mat, idx});
		}
	}
	for (int i = 0; i < n->getNumChildren(); ++i)
		collect_surfaces(n->getChild(i));
}
