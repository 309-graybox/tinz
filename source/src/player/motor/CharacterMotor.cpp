#include "CharacterMotor.h"

#include "utils/Utils.h"
#include "player/input/PlayerInput.h"

#include <UnigineWorld.h>
#include <UnigineGame.h>
#include <UnigineVisualizer.h>

#define DEBUG_MOVEMENT

using namespace Unigine;
using namespace Unigine::Math;

REGISTER_COMPONENT(CharacterMotor);

void CharacterMotor::init()
{
	FLOGERR(target, "target is not set\n");

	ObjectPtr obj = checked_ptr_cast<Object>(body.get());
	FLOGERR(obj, "can't get object from \"body\"\n");

	// BodyDummy — no physics simulation, just collision detection
	_body = checked_ptr_cast<BodyDummy>(obj->getBody());
	FLOGERR(_body, "can't get BodyDummy from \"body\"\n");

	for (int i = 0; i < _body->getNumShapes(); ++i)
		if (!_shape)
			_shape = checked_ptr_cast<ShapeCapsule>(_body->getShape(i));

	FLOGERR(_shape, "can't get ShapeCapsule from \"body\"\n");

	_input.init();

	_player_ifps = 1.0f / playerFps;
	_shape_height = _shape->getHeight();
	_slope_cos = Math::cos(slopeLimit * Consts::DEG2RAD);

	_world_transform = obj->getWorldTransform();

#ifdef DEBUG_MOVEMENT
	Visualizer::setEnabled(true);
#endif
}

void CharacterMotor::update()
{
	float ifps = Game::getIFps() * Physics::getScale();

	// TODO: update input in PlayerInput;
	auto ground_normal = get_ground_normal();
	if (ground_normal == vec3_zero)
		ground_normal = vec3_up;
	_input.update(ground_normal);

	// jump
	_vertical_move = 0.0f;
	if (_input.consumeJump() && _is_grounded)
	{
		_vertical_move = jumpPower / ifps;
	// crouch just for now
	} else if (_input.isCrouching() && !_is_grounded)
	{
		_vertical_move = -jumpPower / ifps;
	}

	_world_transform = target->getWorldTransform();

	vec3 move_dir = _input.getMoveDirection();

	float speed = runSpeed;
	if (_input.isSprinting())
		speed = sprintSpeed;
	else if (_input.isWalking())
		speed = walkSpeed;

	_horizontal_velocity = Vec3(move_dir * speed);

	float slope_cos = dot(move_dir, vec3_up);
	float move_coeff = 1.0f - slope_cos;

#ifdef DEBUG_MOVEMENT
	auto p0 = _world_transform.getTranslate();
	Visualizer::renderVector(p0, p0 + Vec3(move_dir), vec4_green);
	Visualizer::renderVector(p0, p0 + Vec3(ground_normal), vec4_red);
	Visualizer::renderMessage3D(p0 + Vec3_up * 2, vec3_zero, "_is_grounded", _is_grounded ? vec4_green : vec4_red);

	auto pl = Game::getPlayer();
	auto pl_pos = pl->getWorldPosition();
	WorldIntersectionPtr wi = WorldIntersection::create();
	World::getIntersection(pl_pos, pl_pos + Vec3(pl->getViewDirection()) * 1000.0f, ~0, {body}, wi);
	Visualizer::renderPoint3D(wi->getPoint(), 0.1f, vec4_white);
	Visualizer::renderCapsule(_shape->getRadius(), _shape->getHeight(), _shape->getTransform(), vec4_green);
#endif

	float update_time = ifps;
	_is_grounded = false;
	while (update_time > 0.0f)
	{
		float adaptive_time_step = min(update_time, _player_ifps);
		update_time -= adaptive_time_step;

		update_velocity(adaptive_time_step);
		mul(_world_transform, translate((_horizontal_velocity * move_coeff + Vec3_up * _vertical_velocity) * adaptive_time_step), _world_transform);

		Vec3 saved_velocity = _horizontal_velocity;
		resolve_collisions(adaptive_time_step);
		_horizontal_velocity = saved_velocity;

		rotate(move_dir, adaptive_time_step);
	}

	target->setWorldTransform(_world_transform);
	body->setWorldTransform(target->getWorldTransform());
}

// TODO(vah) looks suspicious, optimize
void CharacterMotor::rotate(const vec3 &direction, float dt)
{
	float len2 = direction.x * direction.x + direction.y * direction.y;
	if (compare(len2, 0.0f))
		return;

	float yaw = atan2(-direction.x, direction.y) * Consts::RAD2DEG;
	quat target_rot = quat(0, 0, yaw);
	quat current_rot = _world_transform.getRotate();

	float t = clamp(turnSpeed * dt, 0.0f, 1.0f);
	_world_transform = Mat4(slerp(current_rot, target_rot, t), _world_transform.getTranslate());
}

vec3 CharacterMotor::get_ground_normal() const
{
	vec3 normal;

	Vec3 pos = _shape->getBottomCap();
	float radius = _shape->getRadius();
	Vec3 down_ray = Vec3_down * radius * groundCheckRaysLength;

	auto t = _shape->getTransform();

	Vec3 axes[5] = {Vec3_zero, t.getAxisX(), -t.getAxisX(), t.getAxisY(), -t.getAxisY()};
	WorldIntersectionNormalPtr hit_normal = WorldIntersectionNormal::create();
	for (int i = 0; i < 5; ++i)
	{
		Vec3 p0 = pos + axes[i] * radius;
		auto object = World::getIntersection(p0, p0 + down_ray, groundCheckIntersectionMask, {body}, hit_normal);

#ifdef DEBUG_MOVEMENT
		Visualizer::renderVector(p0, p0 + down_ray, vec4_blue, 0.01f);
#endif

		if (object)
		{
#ifdef DEBUG_MOVEMENT
		Visualizer::renderPoint3D(hit_normal->getPoint(), 0.01f, vec4_blue, false, 0.0f, false);
#endif
			normal += hit_normal->getNormal();
		}
	}

	// UNIGINE_ASSERT(normal != vec3_zero);
	return normal.normalize();
}

void CharacterMotor::update_velocity(float ifps)
{
	_vertical_velocity += _vertical_move * ifps;
	if (!_is_grounded)
		_vertical_velocity += Physics::getGravity().z * ifps;
}

void CharacterMotor::resolve_collisions(float ifds)
{
	for (int iter = 0; iter < collisionIterations; ++iter)
	{
		_body->setTransform(_world_transform);

		_shape->getCollision(_contacts, ifds);
		if (_contacts.size() == 0)
			break;

		int count = min(16, _contacts.size());
		float inum = 1.0f / toFloat(count);

		// Push out of penetration
		vec3 pos_offset = vec3_zero;

		for (int i = 0; i < count; ++i)
		{
			const ShapeContactPtr &c = _contacts[i];
			vec3 normal = c->getNormal();
			float depth = c->getDepth();

			pos_offset += normal * depth * inum;

			// Cancel velocity into surface
			float normalSpeed = toFloat(dot(Vec3(normal), _horizontal_velocity));
			if (normalSpeed < 0.0f)
				_horizontal_velocity -= Vec3(normal) * normalSpeed;

			// Ground detection: contact below capsule center + surface angle
			Vec3 contactPoint = c->getPoint();
			Vec3 bottomCap = _shape->getBottomCap();

#ifdef DEBUG_MOVEMENT
			Visualizer::renderPoint3D(contactPoint, 0.01f, vec4_red);
			Visualizer::renderVector(contactPoint, contactPoint + Vec3(normal), vec4_black);
#endif

			if (dot(contactPoint - bottomCap, Vec3_up) < 0.0f)
			{
				if (dot(normal, vec3_up) > _slope_cos)
				{
					_is_grounded = true;
					if (_vertical_velocity < 0.0f)
						_vertical_velocity = 0.0f;
				}
			}

			// Ceiling detection
			if (dot(normal, vec3_down) > _slope_cos)
			{
				if (_vertical_velocity > 0.0f)
					_vertical_velocity = 0.0f;
			}
		}

		_world_transform.setColumn3(3, _world_transform.getTranslate() + Vec3(pos_offset));
	}
}
