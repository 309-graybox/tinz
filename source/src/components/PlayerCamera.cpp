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

	// 1) ЛАГАЕМ ПИВОТ (а не камеру)
	Vec3 target = rawTarget;
	if (position_lag_enabled)
		target = position_lag->update(rawTarget, dt);

	// 2) ЛАГАЕМ УГОЛ
	vec2 angle = _angle;
	if (rotation_lag_enabled)
		angle = rotation_lag->update(angle, dt);

	float yaw = angle.x * Consts::DEG2RAD;
	float pitch = angle.y * Consts::DEG2RAD;

	Vec3 dir;
	dir.x = cos(yaw) * cos(pitch);
	dir.y = sin(yaw) * cos(pitch);
	dir.z = sin(pitch);

	float dist = cfg->distance_range.get().y;

	Vec3 desiredCam = target + dir * dist;

	// 3) КОЛЛИЗИЯ РЕЖЕТ КАМЕРУ (не target)
	Vec3 cam = desiredCam;
	if (collision_enabled)
		cam = collision->update(desiredCam, target, dt);

	float d = length(cam - target);
	const float minDist = 0.75f;
	if (d < minDist)
		cam = target + normalize(desiredCam - target) * minDist;

	setPosition(cam);

	_player->worldLookAt(target);

	if (debug)
		Visualizer::renderPoint3D(target, 0.05f, vec4_red);
}

void PlayerCamera::shutdown()
{
}
