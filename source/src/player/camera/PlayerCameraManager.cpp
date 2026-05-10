#include "PlayerCameraManager.h"
#include "player/camera/CameraTarget.h"
#include "utils/Utils.h"
#include <UnigineConsole.h>
#include <UnigineGame.h>
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

REGISTER_COMPONENT(PlayerCameraManager)

using namespace Unigine;
using namespace Unigine::Math;

class CameraStageComparator
{
public:
	bool operator()(const CameraStageModifier *a, const CameraStageModifier *b) const
	{
		int sa = (int)a->stage;
		int sb = (int)b->stage;
		if (sa != sb)
			return sa < sb;

		int pa = a->priority;
		int pb = b->priority;
		if (pa != pb)
			return pa < pb;

		return a < b;
	}
};

void PlayerCameraManager::rebuildPipeline()
{
	mods.clear();

	getComponentsInChildren(node, mods);
	mods.erase(std::remove_if(mods.begin(), mods.end(), [](auto *m) { return !m->isActive(); }), mods.end());
	std::sort(mods.begin(), mods.end(), CameraStageComparator());

	_ctx.target = target_node.get();
	_ctx.cameraNode = camera_node.get();
	_ctx.collision_mask = collision_mask.get();

	Vector<String> names;
	for (auto &mod : mods)
	{
		mod->runtimeReset(_state, _ctx);
		names.append(mod->getClassName());
	}

	Log::error("PlayerCameraManager pipeline: %s\n", String::join(names, " -> ").get());
}

void PlayerCameraManager::init()
{
	Input::setMouseHandle(Input::MOUSE_HANDLE_GRAB);

	FLOGERR(target_node, "target_node should be Player\n");

	// Something is broken
	CameraTarget *target = getComponent<CameraTarget>(target_node, true);
	if (!target)
		target = addComponent<CameraTarget>(target_node);
	target->setManager(this);

	rebuildPipeline();

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	auto ei = EISystem::get();
	if (!ei)
		return;

	auto player = getComponent<EILocalPlayer>(node);
	if (!player)
		return;

	auto actionReg = ei->getActionRegistry();

	auto _actionLook = actionReg->create(actionReg->getIndexByPath(action_look_file));
	if (!_actionLook)
	{
		Log::error("PlayerCameraManager::init: action \"%s\" not found\n", action_look_file.get());
		removeComponent<PlayerCameraManager>(node);
		return;
	}

	_bindingLook = player->bind(_actionLook, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance v) {
		_input.angle = {0, 0};
		_input.scroll = 0;
		if (!Console::isActive() && Input::isMouseGrab())
		{
			_input.angle = {v.x(), -v.y()};
			_input.scroll = v.z();
		}

		if (abs(_input.angle.x) < 0.001)
			_input.angle.x = 0;
		if (abs(_input.angle.y) < 0.001)
			_input.angle.y = 0;
		if (abs(_input.scroll) < 0.001)
			_input.scroll = 0;
	});

	auto _actionTargetLock = actionReg->create(actionReg->getIndexByPath(action_target_lock_file));
	if (!_actionTargetLock)
	{
		Log::error("PlayerCameraManager::init: action \"%s\" not found\n", action_look_file.get());
		removeComponent<PlayerCameraManager>(node);
		return;
	}
	_bindingTargetLock = player->bind(_actionTargetLock, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance v) {
		bool pressed = !Console::isActive() && Input::isMouseGrab() && !compare(v.getValue().getMagnitude2(), 0.0f);
		if (pressed && !_targetLockHeld)
			_input.targetLock = true;
		_targetLockHeld = pressed;
	});

	if (_ctx.target)
		_ctx.targetOldPosition = _ctx.target->getWorldPosition();
}

void PlayerCameraManager::update()
{
	update_camera_state();
	update_camera_context();

	for (auto &m : mods)
		m->apply(_state, _input, _ctx);

	_input.targetLock = false;

	if (!_ctx.cameraNode)
		return;

	_ctx.cameraNode->setWorldPosition(_state.pos);
	_ctx.cameraNode->setWorldRotation(_state.rot);

	if (auto cam = Unigine::checked_ptr_cast<Unigine::Player>(_ctx.cameraNode))
	{
		cam->setFov(_state.fov);
	}

	_ctx.targetOldPosition = _ctx.target->getWorldPosition();
}

void PlayerCameraManager::addTrauma(float amount)
{
	if (amount <= 0.0f)
		return;
	_state.trauma = clamp(_state.trauma + amount, 0.0f, 1.0f);
}

void PlayerCameraManager::shutdown()
{
	auto ei = EISystem::get();
	if (!ei)
		return;

	auto player = getComponent<EILocalPlayer>(node);
	if (!player)
		return;

	player->unbind(_bindingTargetLock);
	player->unbind(_bindingLook);
}

void PlayerCameraManager::update_camera_state()
{
	_state.dt = Game::getIFps();
}

void PlayerCameraManager::update_camera_context()
{
	_ctx.target = target_node.get();
	_ctx.cameraNode = camera_node.get();
	_ctx.collision_mask = collision_mask.get();
	if (_ctx.target)
	{
		_ctx.targetPosition = _ctx.target->getWorldPosition();
		_ctx.targetVelocity = (_ctx.targetPosition - _ctx.targetOldPosition) / _state.dt;
		_ctx.targetSpeed = _ctx.targetVelocity.length();
		_ctx.targetSpeedDir = _ctx.targetSpeed != 0 ? _ctx.targetVelocity / _ctx.targetSpeed : Vec3_zero;
		_ctx.targetHorizontalVelocity = Vec3(_ctx.targetVelocity.x, _ctx.targetVelocity.y, 0);
		_ctx.targetHorizontalSpeed = _ctx.targetHorizontalVelocity.length();
		_ctx.targetHorizontalSpeedDir = _ctx.targetHorizontalSpeed != 0 ? _ctx.targetHorizontalVelocity / _ctx.targetHorizontalSpeed : Vec3_zero;
		_ctx.targetVerticalVelocity = Vec3(0, 0, _ctx.targetVelocity.z);
		_ctx.targetVerticalSpeed = _ctx.targetVerticalVelocity.length();
		_ctx.targetVerticalSpeedDir = _ctx.targetVerticalSpeed != 0 ? _ctx.targetVerticalVelocity / _ctx.targetVerticalSpeed : Vec3_zero;
	} else
	{
		_ctx.targetPosition = Vec3_zero;
		_ctx.targetVelocity = Vec3_zero;
		_ctx.targetSpeedDir = Vec3_zero;
		_ctx.targetSpeed = 0;
		_ctx.targetHorizontalVelocity = Vec3_zero;
		_ctx.targetHorizontalSpeedDir = Vec3_zero;
		_ctx.targetHorizontalSpeed = 0;
		_ctx.targetVerticalVelocity = Vec3_zero;
		_ctx.targetVerticalSpeedDir = Vec3_zero;
		_ctx.targetVerticalSpeed = 0;
	}
}
