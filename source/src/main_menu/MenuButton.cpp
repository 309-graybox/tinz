#include "MenuButton.h"
#include "audio/SoundManager.h"

#include <UnigineGame.h>

REGISTER_COMPONENT(MenuButton);

using namespace Unigine;
using namespace Math;

void MenuButton::onUpdate()
{
	if (!node)
		return;

	Vec3 target = _rest_pos;
	if (_pressed)
		target += Vec3(clickOffset.get());
	else if (isHovered())
		target += Vec3(hoverOffset.get());

	const Vec3 cur = node->getPosition();
	const float t = saturate((float)easeSpeed * Game::getIFps());
	node->setPosition(lerp(cur, target, t));
}

void MenuButton::press()
{
	if (_pressed)
		return;
	_pressed = true;

	const char *sfx = clickSound.get();
	if (sfx && *sfx)
		audio::SoundManager::play2D(sfx);
}
