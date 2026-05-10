#pragma once
#include <UnigineComponentSystem.h>

class PlayerCameraManager;

class CameraTarget final: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CameraTarget, Unigine::ComponentBase)

	PlayerCameraManager *getManager() const noexcept { return _manager; }

private:
	void setManager(PlayerCameraManager *manager);

private:
	PlayerCameraManager *_manager = nullptr;

	friend class PlayerCameraManager;
};
