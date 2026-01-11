#include "PlayerCamera.h"
#include <UnigineInput.h>
#include <UnigineVisualizer.h>
#include <UnigineConsole.h>
#include <UnigineGame.h>

REGISTER_COMPONENT(PlayerCamera)

using namespace Unigine;
using namespace Unigine::Math;

static Vec3 solveLag(const Vec3 &current, Vec3 &velocity, const Vec3 &target, float speed, float damping, float dt)
{
	const float omega = speed * 2.0f * Consts::PI;
	const float x = omega * dt;
	const float exp = Math::exp(-damping * x);

	Vec3 delta = current - target;
	Vec3 temp = (velocity + delta * omega) * dt;

	velocity = (velocity - temp * omega) * exp;
	return target + (delta + temp) * exp;
}

static float smoothDamp(float current, float target, float &velocity, float smoothTime, float deltaTime)
{
	smoothTime = max(0.0001f, smoothTime);
	float omega = 2.0f / smoothTime;

	float x = omega * deltaTime;
	float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

	float change = current - target;
	float temp = (velocity + omega * change) * deltaTime;
	velocity = (velocity - omega * temp) * exp;

	return target + (change + temp) * exp;
}

void PlayerCamera::addRotation(const Unigine::Math::vec2 &angle)
{
	vec2 inverseMult(cfg->inverse_x ? 1 : -1, cfg->inverse_y ? -1 : 1);
	_angle += angle * cfg->sensitivity * inverseMult;
	_angle.y = clamp(_angle.y, cfg->pitch_range.get().x, cfg->pitch_range.get().y);

	auto yaw = _angle.x * Consts::DEG2RAD;
	auto pitch = _angle.y * Consts::DEG2RAD;
	_offset.x = cos(yaw) * cos(pitch);
	_offset.y = sin(yaw) * cos(pitch);
	_offset.z = sin(pitch);
}

Vec3 PlayerCamera::clampLag(const Vec3 &lagPos, const Vec3 &target)
{
	Vec3 delta = lagPos - target;

	if (lag->separate_axis)
	{
		delta.x = clamp(delta.x, -lag->max_distance_axis.get().x, lag->max_distance_axis.get().x);
		delta.y = clamp(delta.y, -lag->max_distance_axis.get().y, lag->max_distance_axis.get().y);
		delta.z = clamp(delta.z, -lag->max_distance_axis.get().z, lag->max_distance_axis.get().z);
	} else
	{
		float len = length(delta);
		if (len > lag->max_distance)
			delta *= lag->max_distance / len;
	}

	return target + delta;
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

	_lag_target_pos = getTargetPosition();
	_lag_target_vel = Vec3_zero;

	_lag_angle = _angle;
	_lag_angle_vel = vec2_zero;
}

void PlayerCamera::update()
{
	const float dt = Game::getIFps();

	// ---------- INPUT ----------
	auto mouse = vec2(Input::getMouseDeltaPosition());
	addRotation(mouse);

	// ---------- FOLLOW LAG (target) ----------
	Vec3 rawTargetPos = getTargetPosition();

	if (lag->enabled)
	{
		_lag_target_pos = solveLag(
			_lag_target_pos,
			_lag_target_vel,
			rawTargetPos,
			lag->speed,
			lag->damping,
			dt);
	} else
	{
		_lag_target_pos = rawTargetPos;
		_lag_target_vel = Vec3_zero;
	}

	// ---------- ROTATION LAG (angles) ----------
	if (lag->enabled)
	{
		_lag_angle.x = smoothDamp(
			_lag_angle.x,
			_angle.x,
			_lag_angle_vel.x,
			1.0f / max(lag->rotation_speed, 0.001f),
			dt);

		_lag_angle.y = smoothDamp(
			_lag_angle.y,
			_angle.y,
			_lag_angle_vel.y,
			1.0f / (max(lag->rotation_speed, 0.001f) * 2.5f),
			dt);
	} else
	{
		_lag_angle = _angle;
		_lag_angle_vel = vec2_zero;
	}

	// ---------- ORBIT OFFSET ----------
	float yaw = _lag_angle.x * Consts::DEG2RAD;
	float pitch = _lag_angle.y * Consts::DEG2RAD;

	Vec3 offset;
	offset.x = cos(yaw) * cos(pitch);
	offset.y = sin(yaw) * cos(pitch);
	offset.z = sin(pitch);

	// ---------- FINAL POSITION ----------
	float distance = cfg->distance_range.get().y;
	Vec3 finalPos = _lag_target_pos + offset * distance;

	setPosition(finalPos);

	// ---------- ROTATION ----------
	_player->worldLookAt(_lag_target_pos);

	// ---------- DEBUG ----------
	if (debug)
	{
		Visualizer::renderPoint3D(_lag_target_pos, 0.05f, vec4_red);
	}
}

void PlayerCamera::shutdown()
{
}
