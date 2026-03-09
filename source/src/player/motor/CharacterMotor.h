#pragma once
#include "player/input/PlayerInput.h"

#include <UnigineMathLib.h>
#include <UnigineNode.h>
#include <UnigineObjects.h>
#include <UniginePhysics.h>
#include <UnigineComponentSystem.h>

class CharacterMotor : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CharacterMotor, Unigine::ComponentBase);
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)

	PROP_PARAM(Float, walkSpeed, 2.0f, "", "Скорость персонажа в режиме ходьбы, без влияния модификаторов");
	PROP_PARAM(Float, runSpeed, 5.0f, "", "Cкорость персонажа в режиме бега, без влияния модификаторов");
	PROP_PARAM(Float, acceleration, 15.0f, "", "Набор скорости при беге и ходьбе");
	PROP_PARAM(Float, damping, 23.0f, "", "Потеря скорости при остановке бега или ходьбы");
	PROP_PARAM(Float, turnSpeed, 600.0f, "", "Скорость поворота персонажа в сторону целевого направления во время ходьбы и бега");
	PROP_PARAM(Float, jumpPower, 6.0f, "", "");

	PROP_GROUP("Sprint");
	PROP_PARAM(Float, sprintSpeed, 8.0f, "", "Максимальная скорость персонажа в режиме спринта");
	PROP_PARAM(Float, sprintAcceleration, 12.0f, "", "Набор скорости в режиме спринта");
	PROP_PARAM(Float, sprintDamping, 30.0f, "", "Потеря скорости при выходе из спринта");
	PROP_PARAM(Float, sprintTurnSpeed, 400.0f, "", "Скорость изменения направления движения во время спринта");
	PROP_PARAM(Float, sharpTurnAngleThreshold, 120.f, "", "Минимальный угол изменения направления, при превышении которого активируется резкий разворот в спринте");

	PROP_GROUP("");
	PROP_PARAM(Float, stepHeight, 0.3f, "", "Максимальная высота препятствия, на которое персонаж может автоматически подняться");
	PROP_PARAM(Float, slopeLimit, 43.0f, "Slope Limit(degrees)", "Максимальный угол наклона поверхности, по которой персонаж может двигаться");

	PROP_GROUP("")
	PROP_PARAM(Node, target);
	PROP_PARAM(Node, body);
	PROP_PARAM(Float, groundCheckRaysLength, 1.2f, "", "Длина лучей для проверки нормали поверхности");
	PROP_PARAM(Mask, groundCheckIntersectionMask, ~0, "", "Маска для проверки нормали поверхности");
	PROP_PARAM(Int, playerFps, 60, "Player Fps", "Minimum update rate for the player (in number of frames per second).\n If this value exceeds the current framerate, the player will be updated several times per frame");
	PROP_PARAM(Int, collisionIterations, 4, "Collision Iterations", "Number of iterations to resolve collision");

	void init();
	void update();

	bool isOnGround() const noexcept { return _is_grounded; }

private:
	void rotate(const Unigine::Math::vec3 &direction, float dt);
	Unigine::Math::vec3 get_ground_normal() const;
	void update_velocity(float ifps);
	void resolve_collisions(float ifps);

	PlayerInput _input;

	float _slope_cos = 0.0f;
	float _player_ifps = 1.0f / 60.0f;

	Unigine::BodyDummyPtr _body;
	Unigine::ShapeCapsulePtr _shape;
	float _shape_height;
	Unigine::Vector<Unigine::ShapeContactPtr> _contacts;

	Unigine::Math::Mat4 _world_transform = Unigine::Math::Mat4_identity;
	Unigine::Math::Vec3 _horizontal_velocity = Unigine::Math::Vec3_zero;
	float _vertical_velocity = 0.0f;
	float _vertical_move = 0.0f;

	bool _is_grounded = false;
};
