#include "ThirdPersonCamera.h"
#include "Targetable.h"
#include <UnigineConsole.h>
#include <UnigineVisualizer.h>
#include <UnigineGame.h>

REGISTER_COMPONENT(ThirdPersonCamera)

using namespace Unigine;
using namespace Unigine::Math;

void ThirdPersonCamera::init()
{
	Input::setMouseHandle(Input::MOUSE_HANDLE_GRAB);
	Input::setMouseGrab(true);

	_player = checked_ptr_cast<PlayerDummy>(node);
	UNIGINE_ASSERT(_player);

	if (target)
		updateRotationAfterLock();

	updateCameraOffset();
	updateFreeCamera();
	_currentCameraPos = _cameraPos;
	_currentLockPos = _lockPos;

	_targetFinder = TargetFinderBoundFrustum::create();
	_targetComparator = TargetComparatorNearest::create();
}

void ThirdPersonCamera::update()
{
	if (!_player)
		return;

	if (Input::isMouseButtonDown(Input::MOUSE_BUTTON_MIDDLE))
		searchTargetForLock();

	bool wasLocked = _wasLocked;
	if (!_lockTarget)
	{
		updateFreeCamera();
	} else
	{
		Visualizer::renderPoint3D(_lockTarget->getWorldPosition(), 0.05f, vec4_red, true, 0, false);
		Visualizer::renderPoint3D(_lockPos, 0.05f, vec4_green, true, 0, false);
		updateLockCamera();
	}
	bool nowLocked = _lockTarget != nullptr;

	updateCameraPosCollision();

	float cameraLerpFactor = _lockTarget ? 0.15f : 0.1f;
	float lockLerpFactor = _lockTarget ? 0.08f : 0.05f;

	if (wasLocked != nowLocked)
	{
		_lastCameraPos = _currentCameraPos;
		_lastLockPos = _currentLockPos;
		_lockTransitionTime = 0.0f;
	}

	if (_lockTransitionTime < 1.0f)
	{
		_lockTransitionTime += Game::getIFps() * (1.0f / transition_time);
		_lockTransitionTime = clamp(_lockTransitionTime, 0.0f, 1.0f);

		float smoothT = smoothstep(0.0f, 1.0f, _lockTransitionTime);

		_currentCameraPos = lerp(_lastCameraPos, _cameraPos, smoothT);
		_currentLockPos = lerp(_lastLockPos, _lockPos, smoothT);
	} else
	{
		_currentCameraPos = lerp(_currentCameraPos, _cameraPos, cameraLerpFactor);
		_currentLockPos = lerp(_currentLockPos, _lockPos, lockLerpFactor);
	}

	_player->setWorldPosition(_currentCameraPos);
	_player->worldLookAt(_currentLockPos);
}

void ThirdPersonCamera::updateCameraOffset()
{
	_cameraOffset.x = cos(_yaw * Consts::DEG2RAD) * cos(_pitch * Consts::DEG2RAD);
	_cameraOffset.y = sin(_yaw * Consts::DEG2RAD) * cos(_pitch * Consts::DEG2RAD);
	_cameraOffset.z = sin(_pitch * Consts::DEG2RAD);
}

void ThirdPersonCamera::searchTargetForLock()
{
	if (_lockTarget)
	{
		_lockTarget = nullptr;
		return;
	}

	_targets = _targetFinder->scan({_player, lock_distance});
	sort(_targets, _player, _targetComparator.get());

	if (!_targets.empty())
		_lockTarget = _targets[0]->getTarget();

	_lockTargetIndex = 0;
}

void ThirdPersonCamera::updateRotationAfterLock()
{
	_wasLocked = false;

	auto to_camera = _player->getWorldPosition() - target->getWorldPosition();
	if (to_camera.length() > 0.001f)
	{
		to_camera = normalize(to_camera);
		_yaw = atan2(to_camera.y, to_camera.x) * Consts::RAD2DEG;
		float horizontal_dist = sqrt(to_camera.x * to_camera.x + to_camera.y * to_camera.y);
		_pitch = atan2(to_camera.z, horizontal_dist) * Consts::RAD2DEG;
	} else
	{
		_yaw = 180.0f;
		_pitch = 0.0f;
	}

	updateCameraOffset();
}

void ThirdPersonCamera::updateFreeCamera()
{
	if (_wasLocked)
		updateRotationAfterLock();

	if (!Console::isActive() && Input::isMouseGrab())
	{
		auto mouseDelta = vec2(Input::getMouseDeltaPosition()) * sensitivity;
		if (mouseDelta != vec2_zero)
		{
			_yaw -= mouseDelta.x * sensitivity;
			_pitch += mouseDelta.y * sensitivity;
			_pitch = clamp(_pitch, min_pitch, max_pitch);

			updateCameraOffset();
		}
	}

	auto targetPos = target->getWorldPosition();

	auto dist = distance(target->getWorldPosition(), node->getWorldPosition());
	dist = clamp(dist, min_distance, max_distance);

	_cameraPos = targetPos + _cameraOffset * dist;
	_lockPos = targetPos + offset;
}

void ThirdPersonCamera::updateLockCamera()
{
	_wasLocked = true;

	auto targetPos = target->getWorldPosition() + offset;
	auto lockPos = _lockTarget->getWorldPosition();

	if (!Console::isActive() && Input::isMouseGrab())
	{
		auto mouseDelta = vec2(Input::getMouseDeltaPosition()) * sensitivity;

		// FIXME
		// When you keep moving Unigine does not clear delta
		// So after 3.0 you will receave 3+ and it will cause infinite target changing
		if (mouseDelta.x > 3.0f)
		{
			Log::message("Delta pos %f %f\n", mouseDelta.x, mouseDelta.y);
			_lockTargetIndex = (_lockTargetIndex + 1) % _targets.size();
			_lockTarget = _targets[_lockTargetIndex]->getNode();
		} else if (mouseDelta.x < -3.0f)
		{
			Log::message("Delta neg %f %f\n", mouseDelta.x, mouseDelta.y);
			_lockTargetIndex = (_lockTargetIndex - 1 + _targets.size()) % _targets.size();
			_lockTarget = _targets[_lockTargetIndex]->getNode();
		}
	}

	// FIXME
	// if (!_targetFinder->isInside(lockPos, {_player, unlock_distance}))
	// {
	// 	Log::message("Target out\n");
	// 	auto ci =
	// 	_lockTargetIndex = (_lockTargetIndex + 1) % _targets.size();
	// 	auto nt = _targets[_lockTargetIndex];

	// 	_lockTarget = _targets[_lockTargetIndex]->getNode();
	// 	return;
	// }

	auto dir = normalize(targetPos - lockPos);
	auto cameraOffset = dir * min_distance + Vec3(0, 0, offset.get().z);

	_cameraPos = targetPos + cameraOffset;
	_lockPos = targetPos - dir * distance(targetPos, lockPos) * lock_shfit;
}

void ThirdPersonCamera::updateCameraPosCollision()
{
	Vec3 targetPos = target->getWorldPosition() + offset;

	Vec3 cameraToTarget = targetPos - _cameraPos;

	WorldIntersectionPtr intersection = WorldIntersection::create();
	auto hit = World::getIntersection(targetPos, _cameraPos, intersection_mask, intersection);

	if (hit)
	{
		auto hitPoint = intersection->getPoint();
		auto hitToTarget = targetPos - hitPoint;
		double safeDistance = 0.3;

		_cameraPos = hitPoint + normalize(hitToTarget) * safeDistance;

		auto dist = distance(targetPos, _cameraPos);

		if (dist < 0.5)
			_cameraPos = targetPos - normalize(cameraToTarget) * 0.5;
	}
}
