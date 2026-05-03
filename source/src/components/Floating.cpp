#include "Floating.h"

#include <UnigineComponentSystem.h>
#include <UnigineGame.h>

using namespace Unigine;
using namespace Math;

REGISTER_COMPONENT(Floating);

void Floating::init()
{
	_base_pos = node->getPosition();
}

void Floating::update()
{
	float ifps = Game::getIFps();

	_time += ifps * speed;

	float offset = sin(_time) * amplitude;

	Vec3 pos = node->getPosition();

	pos.z = _base_pos.z + offset;

	node->setPosition(pos);
}