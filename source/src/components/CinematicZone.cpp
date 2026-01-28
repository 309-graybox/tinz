#include "CinematicZone.h"
#include "CameraTarget.h"
#include "player/camera/stages/CameraCinematicZone.h"
#include "player/camera/PlayerCameraManager.h"
#include "utils/Utils.h"

REGISTER_COMPONENT(CinematicZone)

using namespace Unigine;
using namespace Unigine::Math;

void CinematicZone::init()
{
	_boundTrigger = checked_ptr_cast<WorldTrigger>(bound_trigger.get());
	FLOGERR(_boundTrigger, "bound_trigger should be WorldTrigger\n");

	_boundTrigger->setExcludeNodes({bound_trigger, target_node});
	_boundTrigger->setTriggerInteractionEnabled(true);

	_boundTrigger->getEventEnter().connect(this, &CinematicZone::onEnter);
	_boundTrigger->getEventLeave().connect(this, &CinematicZone::onLeave);
}

void CinematicZone::update()
{
	_boundTrigger->renderVisualizer();
}

void CinematicZone::onEnter(const Unigine::NodePtr &n)
{
	auto target = getComponent<CameraTarget>(n);
	if (target)
	{
		auto c = getComponentInChildren<CameraCinematicZone>(target->getManager()->getNode());
		if (c)
			c->addZone(this);
	}
}

void CinematicZone::onLeave(const Unigine::NodePtr &n)
{
	auto target = getComponent<CameraTarget>(n);
	if (target)
	{
		auto c = getComponentInChildren<CameraCinematicZone>(target->getManager()->getNode());
		if (c)
			c->removeZone(this);
	}
}
