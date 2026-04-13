#pragma once
#include "MovementContext.h"
#include "MovementState.h"
#include "IdleState.h"
#include "MoveState.h"
#include "TurnState.h"

#include <UnigineComponentSystem.h>
#include <UniginePhysics.h>
#include <UniginePtr.h>

class MovementState;

class CharacterMovement : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CharacterMovement, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_PARAM(Float, walkSpeed, 2.0f, "", "Скорость персонажа в режиме ходьбы, без влияния модификаторов");
	PROP_PARAM(Float, runSpeed, 5.0f, "", "Cкорость персонажа в режиме бега, без влияния модификаторов");
	PROP_PARAM(Float, turnSpeed, 600.0f, "", "Скорость поворота персонажа в сторону целевого направления во время ходьбы и бега");
	PROP_PARAM(Float, jumpPower, 6.0f, "", "");
	PROP_PARAM(Float, turningExitThreshold, 10.0f, "", "Минимальный угол до которого персонаж должен повернуться, чтобы начать идти во время резкого разворота");

	PROP_GROUP("Sprint");
	PROP_PARAM(Float, sprintSpeed, 8.0f, "", "Максимальная скорость персонажа в режиме спринта");
	PROP_PARAM(Float, sprintTurnSpeed, 400.0f, "", "Скорость изменения направления движения во время спринта");
	PROP_PARAM(Float, sharpTurnAngleThreshold, 120.0f, "", "Минимальный угол изменения направления, при превышении которого активируется резкий разворот в спринте");

	PROP_GROUP("");
	PROP_PARAM(Float, stepHeight, 0.3f, "", "Максимальная выс та препятст вия, на которое персонаж может автоматически подняться");
	PROP_PARAM(Float, slopeLimit, 43.0f, "Slope Limit(degr ees)", "Максимальный угол наклона поверхности, по которой персонаж может двигаться");

	PROP_GROUP("")
	PROP_PARAM(Node, body);
	PROP_PARAM(Node, target);
	PROP_PARAM(Float, groundCheckRaysLength, 1.2f, "", "Длина лучей для проверки нормали поверхности");
	PROP_PARAM(Mask, groundCheckIntersectionMask, ~0, "", "Маска для проверки нормали поверхности");
	PROP_PARAM(Int, collisionIterations, 4, "Collision Iterations", "Number of iterations to resolve collision");
	PROP_PARAM(Int, playerFps, 60, "Player Fps", "Minimum update rate for the player (in number of frames per second).\n If this value exceeds the current framerate, the player will be updated several times per frame");
	
private:
	void init();
	void update();
	void shutdown();

public:
	// TODO(vah): obmazat' with inkapsulation?
	void setGravity(const Unigine::Math::Vec3 &gravity);
	// float getSharpTurnCos() const noexcept { return _turning_exit_cos; };

private:
	MovementContext _ctx;
	IdleState _idle_state;
	MoveState _move_state;
	TurnState _turn_state;
	MovementState *_states[MovementStateIndex::COUNT];
	MovementStateIndex _current_state = MovementStateIndex::IDLE;

	Unigine::Math::vec3 get_ground_normal() const;
	Unigine::Math::vec3 compute_desired_input_direction() const;
	Unigine::Math::vec3 project_forward_on_ground(const Unigine::Math::vec3 &ground_normal);
	void resolve_collisions(float ifps);
	void rotate(const Unigine::Math::vec3 &direction, float turn_speed, float ifps);

	float _slope_cos = 0.0f;
	float _player_ifps = 1.0f / 60.0f;

	Unigine::BodyDummyPtr _body;
	Unigine::ShapeCapsulePtr _shape;
	Unigine::Vector<Unigine::ShapeContactPtr> _contacts;

	Unigine::Math::Mat4 _world_transform = Unigine::Math::Mat4_identity;
	Unigine::Math::Vec3 _horizontal_velocity = Unigine::Math::Vec3_zero;
	Unigine::Math::vec3 _gravity_direction;
	Unigine::Math::vec3 _up;
	float _gravity_amount = 0.0f;

	float _vertical_speed = 0.0f;
	float _sharp_turn_cos = 0.0f;
	float _turning_exit_cos = 0.0f;

	bool _is_grounded = false;

	friend class MoveState;
	friend class TurnState;
	friend class IdleState;
};

