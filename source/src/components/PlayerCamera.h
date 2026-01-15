#pragma once
#include "CameraCollision.h"
#include "CameraPositionLag.h"
#include "CameraRotationLag.h"
#include "CameraSpringArm.h"
#include "utils/DebugHelpers.h"
#include <UnigineComponentSystem.h>

class PlayerCamera: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(PlayerCamera, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(Toggle, debug, true)
	PROP_PARAM(Node, target_node)

	struct Cfg: public Unigine::ComponentStruct
	{
		PROP_PARAM(Vec2, sensitivity, {0.2f, 0.2f})
		PROP_PARAM(Toggle, inverse_x, false)
		PROP_PARAM(Toggle, inverse_y, false)
		PROP_PARAM(DVec2, distance_range, {2.0f, 5.0f})
		PROP_PARAM(Vec2, pitch_range, {-89.9f, 89.9f})
	};
	PROP_STRUCT(Cfg, cfg)

	PROP_PARAM(Toggle, collision_enabled, true)
	PROP_STRUCT(CameraCollision, collision)

	PROP_PARAM(Toggle, spring_arm_enabled, true)
	PROP_STRUCT(CameraSpringArm, spring_arm)

	PROP_PARAM(Toggle, position_lag_enabled, true)
	PROP_STRUCT(CameraPositionLag, position_lag)

	PROP_PARAM(Toggle, rotation_lag_enabled, true)
	PROP_STRUCT(CameraRotationLag, rotation_lag)

	void setPosition(const Unigine::Math::Vec3 &p) { _player->setWorldPosition(p); }
	Unigine::Math::Vec3 getPosition() const { return _player->getWorldPosition(); }

	void setRotation(const Unigine::Math::quat &q) { _player->setWorldRotation(q); }
	Unigine::Math::quat getRotation() const { return _player->getWorldRotation(); }

	void setTargetPosition(const Unigine::Math::Vec3 &p) { _target->setWorldPosition(p); }
	Unigine::Math::Vec3 getTargetPosition() const { return _target->getWorldPosition(); }

	void addRotation(const Unigine::Math::vec2 &angle);
	void lookAtTarget() { _player->worldLookAt(getTargetPosition()); }

private:
	Unigine::Math::Vec3 clampLag(const Unigine::Math::Vec3 &lagPos, const Unigine::Math::Vec3 &target);

private:
	void init();
	void update();
	void shutdown();

private:
	VisualizerHelper _vis;
	ConsoleHelper _con;

private:
	Unigine::PlayerDummyPtr _player;
	Unigine::NodePtr _target;
	Unigine::Math::vec2 _angle; // x - yaw, y - pitch
};
