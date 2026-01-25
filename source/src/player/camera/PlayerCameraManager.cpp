#include "PlayerCameraManager.h"
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <UnigineConsole.h>
#include <UnigineGame.h>

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

	rebuildPipeline();

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	auto ei = EISystem::get();

	auto actionReg = ei->getActionRegistry();

	_actionLook = actionReg->create(actionReg->getIndexByPath(action_look_file));
	if (!_actionLook)
	{
		Log::error("PlayerCameraManager::init: action \"%s\" not found\n", action_look_file.get());
		removeComponent<PlayerCameraManager>(node);
		return;
	}

	_bindingLook = ei->bind(_actionLook, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance v) {
		_input.angle = {0, 0};
		_input.scroll = 0;
		if (!Console::isActive() && Input::isMouseGrab())
		{
			_input.angle = {v.x(), v.y()};
			_input.scroll = v.z();
		}
	});

	_actionTargetLock = actionReg->create(actionReg->getIndexByPath(action_target_lock_file));
	if (!_actionLook)
	{
		Log::error("PlayerCameraManager::init: action \"%s\" not found\n", action_look_file.get());
		removeComponent<PlayerCameraManager>(node);
		return;
	}
	_bindingTargetLock = ei->bind(_actionTargetLock, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance v) {
		_input.targetLock = false;
		if (!Console::isActive() && Input::isMouseGrab())
		{
			_input.targetLock = !compare(v.getValue().getMagnitude2(), 0.0f);
		}
	});
}

void PlayerCameraManager::update()
{
	_state.dt = Game::getIFps();

	_ctx.target = target_node.get();
	_ctx.cameraNode = camera_node.get();
	_ctx.collision_mask = collision_mask.get();
	if (_ctx.target)
	{
		_ctx.targetVelocity = (_ctx.target->getWorldPosition() - _ctx.target->getOldWorldPosition()) / _state.dt;
		_ctx.targetSpeed = _ctx.targetVelocity.length();
		_ctx.targetSpeedDir = _ctx.targetSpeed != 0 ? _ctx.targetVelocity / _ctx.targetSpeed : Vec3_zero;
		_ctx.targetHorizontalVelocity = Vec3(_ctx.targetVelocity.x, _ctx.targetVelocity.y, 0);
		_ctx.targetHorizontalSpeed = _ctx.targetHorizontalVelocity.length();
		_ctx.targetHorizontalSpeedDir = _ctx.targetHorizontalSpeed != 0 ? _ctx.targetHorizontalVelocity / _ctx.targetHorizontalSpeed : Vec3_zero;
		_ctx.targetVerticalVelocity = Vec3(0, 0, _ctx.targetVelocity.z);
		_ctx.targetVerticalSpeed = _ctx.targetVerticalVelocity.length();
		_ctx.targetVerticalSpeedDir = _ctx.targetVerticalSpeed != 0 ? _ctx.targetHorizontalVelocity / _ctx.targetVerticalSpeed : Vec3_zero;
	} else
	{
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

	for (auto &m : mods)
		m->apply(_state, _input, _ctx);

	if (!_ctx.cameraNode)
		return;

	_ctx.cameraNode->setWorldPosition(_state.pos);
	_ctx.cameraNode->setWorldRotation(_state.rot);

	if (auto cam = Unigine::checked_ptr_cast<Unigine::Player>(_ctx.cameraNode))
	{
		cam->setFov(_state.fov);
	}
}

void PlayerCameraManager::shutdown()
{
	EISystem::get()->unbind(_actionTargetLock, _bindingTargetLock);
	EISystem::get()->unbind(_actionLook, _bindingLook);
}
