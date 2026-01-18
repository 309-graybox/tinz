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
	_ctx.camera_node = camera_node.get();
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

	auto action = ei->getActionRegistry()->create(ei->getActionRegistry()->getIndexByPath(action_file));
	if (!action)
	{
		Log::error("PlayerCameraManager::init: action \"%s\" not found\n", action_file.get());
		removeComponent<PlayerCameraManager>(node);
		return;
	}

	_binding = ei->bind(action, eTriggerState::Triggered | eTriggerState::None, [this](EIActionValueInstance v) {
		if (!Console::isActive() && Input::isMouseGrab())
		{
			_input.angle = {v.x(), v.y()};
			_input.scroll = v.z();
		} else
		{
			_input.angle = {0, 0};
			_input.scroll = 0;
		}
	});
}

void PlayerCameraManager::update()
{
	_state.dt = Game::getIFps();

	_ctx.target = target_node.get();
	_ctx.camera_node = camera_node.get();
	_ctx.collision_mask = collision_mask.get();

	for (auto &m : mods)
		m->apply(_state, _input, _ctx);

	if (!_ctx.camera_node)
		return;

	_ctx.camera_node->setWorldPosition(_state.pos);
	_ctx.camera_node->setWorldRotation(_state.rot);

	if (auto cam = Unigine::checked_ptr_cast<Unigine::Player>(_ctx.camera_node))
	{
		cam->setFov(_state.fov);
	}
}

void PlayerCameraManager::shutdown()
{
	EISystem::get()->unbind(_action, _binding);
}
