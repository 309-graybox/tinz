#pragma once
#include <UnigineComponentSystem.h>

class ThirdPersonCamera: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(ThirdPersonCamera, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Float, sensitivity, 0.2f)

	PROP_PARAM(Float, min_pitch, 0)
	PROP_PARAM(Float, max_pitch, 180)

#ifdef UNIGINE_DOUBLE
	PROP_PARAM(Double, min_distance, 3.0)
	PROP_PARAM(Double, max_distance, 7.0)
	PROP_PARAM(DVec3, offset, {0.0, 0.0, 1.0})
#else
	PROP_PARAM(Float, min_distance, 3.0f)
	PROP_PARAM(Float, max_distance, 7.0f)
	PROP_PARAM(Vec3, offset, {0.0f, 0.0f, 1.0f})
#endif

	PROP_PARAM(Node, target)
	PROP_PARAM(Float, transition_time, 0.25f)

#ifdef UNIGINE_DOUBLE
	PROP_PARAM(Double, lock_shfit, 0.5)
#else
	PROP_PARAM(Float, lock_shfit, 0.5f)
#endif

	PROP_PARAM(File, lock_image_file)

#ifdef UNIGINE_DOUBLE
	PROP_PARAM(Double, lock_distance, 25.0)
#else
	PROP_PARAM(Float, lock_distance, 25.0f)
#endif

private:
	void init();
	void update();

private:
	void updateCameraOffset();
	void searchTargetForLock();
	void updateRotationAfterLock();
	void updateFreeCamera();
	void updateLockCamera();

private:
	Unigine::PlayerDummyPtr _player;
	Unigine::Math::Vec3 _cameraOffset;
	Unigine::Math::Vec3 _lastCameraPos;
	Unigine::Math::Vec3 _currentCameraPos;
	Unigine::Math::Vec3 _cameraPos;
	Unigine::Math::Vec3 _lastLockPos;
	Unigine::Math::Vec3 _currentLockPos;
	Unigine::Math::Vec3 _lockPos;
	Unigine::NodePtr _lockTarget;
	float _lockTransitionTime = 0.0f;

	float _yaw = 0;
	float _pitch = 0;

	bool _wasLocked = false;
};
