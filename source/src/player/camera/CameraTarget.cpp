#include "player/camera/CameraTarget.h"

REGISTER_COMPONENT(CameraTarget)

void CameraTarget::setManager(PlayerCameraManager *manager)
{
	UNIGINE_ASSERT(manager);

	if (_manager)
		return;

	_manager = manager;
}
