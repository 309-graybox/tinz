#pragma once
#include "MovementContext.h"
#include "MovementState.h"
#include "IdleState.h"
#include "MoveState.h"
#include "SlideState.h"
#include "utils/TimedFlag.h"

#include <UnigineComponentSystem.h>
#include <UniginePhysics.h>
#include <UniginePtr.h>

class MovementState;

class CharacterMovement: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(CharacterMovement, Unigine::ComponentBase);
	COMPONENT_INIT(init);
	COMPONENT_UPDATE(update);
	COMPONENT_SHUTDOWN(shutdown);

	PROP_PARAM(Float, walkSpeed, 2.0f, "", "Скорость персонажа в режиме ходьбы, без влияния модификаторов");
	PROP_PARAM(Float, runSpeed, 5.0f, "", "Cкорость персонажа в режиме бега, без влияния модификаторов");
	PROP_PARAM(Float, turnResponsiveness, 18.0f, "", "Скорость экспоненциального демпфирования поворота к целевому направлению (1/с) во время ходьбы и бега. Чем больше — тем резче поворот. ~18 — snappy старт + плавный выход на цель (~170мс на 95% угла). Меньше = больше инерции");
	PROP_PARAM(Float, fullSpeedAngle, 20.0f, "", "Угол (в градусах) между текущим направлением фейсинга и направлением ввода, при котором персонаж сохраняет полную скорость. На углах меньше этого скорость не уменьшается");
	PROP_PARAM(Float, plantAngle, 70.0f, "", "Угол (в градусах), при котором скорость падает в 0 — персонаж «упирается ногой» и только доворачивается. Между fullSpeedAngle и plantAngle скорость убывает линейно, давая плавное замедление в дугу поворота и плант на ~180°. Сужение до ~60° убирает «скольжение» (тело отстаёт от вектора движения) ценой более частого планта");
	PROP_PARAM(Float, groundAcceleration, 15.0f, "", "Скорость экспоненциального демпфирования набора горизонтальной скорости на земле (1/с). ~15 — выход на ~95% целевой скорости за 0.2с. Меньше — больше «веса» при старте, больше — резче");
	PROP_PARAM(Float, groundDeceleration, 20.0f, "", "Скорость демпфирования торможения горизонтальной скорости на земле (1/с) — когда целевая скорость меньше текущей. ~20 — полная остановка за 0.15с. Применяется и при отпускании ввода, и при планте на резком повороте (когда скорость занулена через plantAngle)");
	PROP_PARAM(Float, jumpPower, 6.0f, "", "");
	PROP_PARAM(Float, jumpBufferTime, 0.15f, "", "Окно (в секундах), в течение которого нажатый прыжок остаётся валидным для срабатывания при первой возможности");
	PROP_PARAM(Float, coyoteTime, 0.2f, "", "Окно (в секундах) после схода с земли, в течение которого прыжок ещё считается возможным");
	PROP_PARAM(Float, adaptiveJumpDamping, 0.6f, "", "Доля, на которую уменьшается текущая вертикальная скорость при отпускании прыжка для адаптивного прыжка");
	PROP_PARAM(Float, adaptiveJumpThreshold, 0.05f, "", "Пороговое значение вертикальной скорости (в долях jumpPower), ниже которого адаптивный прыжок перестаёт применяться");

	PROP_PARAM(Float, airControl, 5.0f, "", "Скорость сведения горизонтальной скорости в воздухе к направлению ввода (1/сек). 0 — полное сохранение импульса (без ввода), выше — быстрее заходим в направление ввода");
	PROP_PARAM(Float, groundedAnimCoyote, 0.1f, "", "Окно (сек) после потери walkable-контакта, в течение которого анимация всё ещё считает персонажа на земле. Гасит микро-разрывы grounded на стыках поверхностей. На прыжке игнорируется");
	PROP_PARAM(Float, slideEntryDelay, 0.05f, "", "Окно (сек) после walkable-контакта, в течение которого вход в скольжение подавляется. Гасит дребезг на стыках плоской и наклонной поверхностей");

	PROP_GROUP("Sprint");
	PROP_PARAM(Float, sprintSpeed, 8.0f, "", "Максимальная скорость персонажа в режиме спринта");
	PROP_PARAM(Float, sprintTurnResponsiveness, 10.0f, "", "Скорость экспоненциального демпфирования поворота во время спринта (1/с). Ниже, чем turnResponsiveness — даёт более широкий радиус разворота на скорости");
	PROP_PARAM(Toggle, alwaysSprint, false, "", "Если включено — простое движение идёт сразу на скорости спринта (и с анимацией спринта), без зажатого инпута спринта. Отдельный инпут спринта при этом остаётся рабочим (просто становится no-op)");

	PROP_GROUP("");
	PROP_PARAM(Float, stepHeight, 0.3f, "", "Максимальная выс та препятст вия, на которое персонаж может автоматически подняться");
	PROP_PARAM(Float, stepClimbSpeed, 5.0f, "Step Climb Speed (m/s)", "Скорость вертикального подъёма при автостеппинге. Гравитация в это время выключена, персонаж считается на земле");
	PROP_PARAM(Float, groundSnapMinGap, 0.02f, "Ground Snap Min Gap (m)", "Минимальный разрыв между низом капсулы и грунтом, при котором срабатывает прибивание к земле при спуске со ступенек. Меньшие разрывы считаются физическим шумом на стыках поверхностей и игнорируются");
	PROP_PARAM(Float, slopeLimit, 43.0f, "Slope Limit(degr ees)", "Максимальный угол наклона поверхности, по которой персонаж может двигаться");

	PROP_GROUP("Slide");
	PROP_PARAM(Float, slideMaxAngle, 70.0f, "Slide Max Angle (degrees)", "Верхняя граница диапазона углов, при которых работает скольжение. Круче — поверхность считается стеной");
	PROP_PARAM(Float, escapeSlideAngle, 40.0f, "Escape Slide Angle (degrees)", "Гистерезис выхода: пока угол поверхности больше этого значения, скольжение продолжается");
	PROP_PARAM(Float, baseSlideSpeed, 4.0f, "Base Slide Speed", "Базовое значение скорости скольжения, к которому применяются модификаторы наклона и ввода");
	PROP_PARAM(Float, slideForwardMultiplier, 1.4f, "", "Множитель скорости при вводе движения по направлению скольжения");
	PROP_PARAM(Float, slideBackMultiplier, 0.6f, "", "Множитель скорости при вводе движения против направления скольжения");
	PROP_PARAM(Float, slideMinSlopeMultiplier, 1.0f, "", "Множитель скорости у нижней границы диапазона скольжения (slopeLimit)");
	PROP_PARAM(Float, slideMaxSlopeMultiplier, 1.8f, "", "Множитель скорости у верхней границы диапазона скольжения (slideMaxAngle)");
	PROP_PARAM(Float, slideAcceleration, 8.0f, "", "Скорость, с которой текущая скорость скольжения подтягивается к расчётной целевой");
	PROP_PARAM(Float, slideLateralStrength, 1.0f, "", "Сила бокового отклонения вектора движения от базового вектора скольжения по инпуту [0..1]");

	PROP_GROUP("Damage");
	PROP_PARAM(Float, damageKnockbackSpeed, 3.5f, "", "Горизонтальная скорость отталкивания при получении урона");
	PROP_PARAM(Float, damageKnockbackDuration, 0.25f, "", "Время затухания отталкивания от урона");

	PROP_GROUP("Push");
	PROP_PARAM(Float, pushStrength, 1.5f, "", "Множитель импульса, передаваемого динамическим физическим телам (BodyRigid) при контакте с персонажем. Импульс пропорционален массе тела и скорости сближения. 0 — не толкать тела");
	PROP_PARAM(Float, pushMaxSpeed, 12.0f, "", "Верхняя граница скорости (м/с), до которой персонаж может разогнать тело своим контактом. Защищает от чрезмерных вылетов лёгких тел при высокой pushStrength. 0 — без ограничения");

	PROP_GROUP("")
	PROP_PARAM(Node, body);
	PROP_PARAM(Float, fall_scale, 0.5f);
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

	// External vertical impulse — overrides the current vertical speed to at
	// least `speed` (m/s, positive = up). Used by stomp-killable enemies for
	// rebound bounce. Doesn't slow the player if they're already going up faster.
	void applyVerticalBounce(float speed);
	void applyDamageKnockback(const Unigine::Math::Vec3 &source_position);

	// Signed vertical speed in m/s along the local up axis. Positive = up.
	// Read by PlayerStomp to gate its hitbox by "currently falling fast enough".
	float getVerticalSpeed() const noexcept { return _vertical_speed; }

	const Unigine::AnimScriptPtr &getAnimScript() const noexcept { return _anim; }

	bool isGrounded() const noexcept { return _is_grounded; }
	MovementStateIndex getMovementState() const noexcept { return _current_state; }
	Unigine::NodePtr getBodyNode() const { return body.get(); }

	void setActionSpeedScale(float scale) noexcept { _action_speed_scale = scale; }
	void setJumpBlocked(bool blocked) noexcept { _jump_blocked = blocked; }

private:
	MovementContext _ctx;
	IdleState _idle_state;
	MoveState _move_state;
	SlideState _slide_state;
	MovementState *_states[MovementStateIndex::COUNT];
	MovementStateIndex _current_state = MovementStateIndex::IDLE;

	Unigine::Math::vec3 get_ground_normal() const;
	Unigine::Math::vec3 compute_desired_input_direction() const;
	Unigine::Math::vec3 project_forward_on_ground(const Unigine::Math::vec3 &ground_normal);
	void resolve_collisions(float ifps);
	void try_auto_step(const Unigine::Math::Mat4 &pre_motion, const Unigine::Math::Vec3 &horiz_motion, float ifps);
	void rotate(const Unigine::Math::vec3 &direction, float turn_responsiveness, float ifps);

	float _slope_cos = 0.0f;
	float _slide_max_cos = 0.0f;
	float _escape_slope_cos = 0.0f;
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
	float _action_speed_scale = 1.0f;
	bool _jump_blocked = false;
	Unigine::Math::Vec3 _damage_knockback_velocity = Unigine::Math::Vec3_zero;
	float _damage_knockback_timer = 0.0f;
	float _damage_knockback_duration = 0.0f;

	bool _is_grounded = false;
	bool _walkable_grounded = false;
	bool _on_steep_slope = false;
	bool _hit_wall = false;
	Unigine::Math::vec3 _hit_wall_normal = Unigine::Math::vec3_zero;
	bool _climbing = false;
	float _climb_target_height = 0.0f;
	float _climb_time = 0.0f;
	bool _descending = false;
	float _descent_target_height = 0.0f;
	float _descent_time = 0.0f;
	float _max_below_slope_dot = 0.0f;
	Unigine::Math::vec3 _steep_slope_normal = Unigine::Math::vec3_up;
	bool _adaptive_jump_pending = false;
	TimedFlag _grounded_flag;
	TimedFlag _walkable_flag;
	TimedFlag _post_climb_flag;

	float _shape_height = 0.0f;
	Unigine::AnimScriptPtr _anim;

private:
	friend class MoveState;
	friend class IdleState;
	friend class SlideState;
};
