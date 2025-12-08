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
	_player = checked_ptr_cast<PlayerDummy>(node);
	UNIGINE_ASSERT(_player);

	if (target)
		updateRotationAfterLock();

	updateCameraOffset();
	updateFreeCamera();
	_currentCameraPos = _cameraPos;
	_currentLockPos = _lockPos;
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

	WorldBoundSphere bs(target->getWorldPosition(), lock_distance);

	Vector<NodePtr> nodes;

	if (!World::getIntersection(bs, nodes) || nodes.empty())
		return;

	auto targetPos = target->getWorldPosition();

	Targetable *nearest = nullptr;
	Scalar nearestDist2 = Consts::INF;

	for (const auto &n : nodes)
	{
		auto targetable = getComponentInChildren<Targetable>(n);
		if (!targetable)
			continue;

		// TODO Check for objects between
		auto dist2 = distance2(n->getWorldPosition(), targetPos);
		if (dist2 < nearestDist2)
		{
			nearest = targetable;
			nearestDist2 = dist2;
		}
	}

	if (nearest)
		_lockTarget = nearest->getTarget();
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

	WorldBoundSphere bs(target->getWorldPosition(), lock_distance);
	if (!bs.inside(lockPos))
	{
		_lockTarget = nullptr;
		return;
	}

	auto dir = normalize(targetPos - lockPos);
	auto cameraOffset = dir * min_distance + Vec3(0, 0, offset.get().z);

	_cameraPos = targetPos + cameraOffset;
	_lockPos = targetPos - dir * distance(targetPos, lockPos) * lock_shfit;
}
