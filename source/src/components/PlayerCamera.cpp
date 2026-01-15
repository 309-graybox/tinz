#include "PlayerCamera.h"
#include <UnigineInput.h>
#include <UnigineVisualizer.h>
#include <UnigineConsole.h>
#include <UnigineGame.h>

REGISTER_COMPONENT(PlayerCamera)

using namespace Unigine;
using namespace Unigine::Math;

void PlayerCamera::addRotation(const Unigine::Math::vec2 &angle)
{
	vec2 inverseMult(cfg->inverse_x ? 1 : -1, cfg->inverse_y ? -1 : 1);
	_angle += angle * cfg->sensitivity * inverseMult;
	_angle.y = clamp(_angle.y, cfg->pitch_range.get().x, cfg->pitch_range.get().y);
}

void PlayerCamera::init()
{
	_vis.setEnabled(debug);
	_con.setOnscreen(debug);

	_player = checked_ptr_cast<PlayerDummy>(node);
	UNIGINE_ASSERT(_player);

	_target = target_node;
	UNIGINE_ASSERT(_target);

	addRotation({0.0f, 0.0f});

	if (position_lag_enabled)
		position_lag->init(getTargetPosition());

	if (rotation_lag_enabled)
		rotation_lag->init(_angle);
}

void PlayerCamera::update()
{
	float dt = Game::getIFps();

	vec2 mouse = vec2(Input::getMouseDeltaPosition());
	addRotation(mouse);

	Vec3 rawTarget = getTargetPosition();

	Vec3 target = rawTarget;
	if (position_lag_enabled)
		target = position_lag->update(rawTarget, dt);

	vec2 angle = _angle;
	if (rotation_lag_enabled)
		angle = rotation_lag->update(angle, dt);

	float yaw = angle.x * Consts::DEG2RAD;
	float pitch = angle.y * Consts::DEG2RAD;

	Vec3 dir;
	dir.x = cos(yaw) * cos(pitch);
	dir.y = sin(yaw) * cos(pitch);
	dir.z = sin(pitch);

	float desiredDist = cfg->distance_range.get().y;

	Vec3 desiredCam = target + dir * desiredDist;

	Vec3 collisionCam = desiredCam;
	float collisionDist = desiredDist;

	if (collision_enabled)
	{
		collisionCam = collision->update(desiredCam, target, dt);
		collisionDist = distance(collisionCam, target);

		const float minDist = 0.75f;
		if (collisionDist < minDist)
			collisionDist = minDist;

		collisionCam = target + dir * collisionDist;
	}

	float finalDist = collisionDist;

	if (spring_arm_enabled)
	{
		finalDist = spring_arm->update(desiredDist, collisionDist, dt);
	}

	Vec3 camPos = target + dir * finalDist;

	setPosition(camPos);
	_player->worldLookAt(target);

	if (debug)
		Visualizer::renderPoint3D(target, 0.05f, vec4_red);
}

void PlayerCamera::shutdown()
{
}
