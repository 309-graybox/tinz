#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraAutoAlignYawRig final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraAutoAlignYawRig, CameraStageModifier);

	PROP_PARAM(Float, input_deadzone, 0.05f)
	PROP_PARAM(Float, delay, 3.0f)

	PROP_PARAM(Float, speed, 10.0f)
	PROP_PARAM(Float, max_deg_per_sec, 180.0f)
	PROP_PARAM(Float, rate_acceleration, 360.0f, "", "Скорость нарастания углового лимита авто-поворота (deg/s^2). 360 — выход на max_deg_per_sec за ~0.5с при max=180. Меньше — мягче старт, меньше дёргает направление движения, завязанное на камеру")
	PROP_PARAM(Toggle, rate_acceleration_exponential, false, "", "Если включено, лимит нарастает экспоненциально: r += (max - r) * (1 - exp(-k*dt)), где k = rate_acceleration / max. Иначе — линейно (r += rate_acceleration * dt)")

	PROP_PARAM(Float, yaw_offset_deg, 0.0f)

	PROP_PARAM(Float, vel_dir_smooth, 12.0f)
	PROP_PARAM(Float, target_yaw_smooth, 10.0f)

	PROP_PARAM(Toggle, reset_timer_on_stop, true)
	PROP_PARAM(Float, min_target_velocity, 2.0f)

	void runtimeReset(CameraState &, const CameraContext &) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

private:
	float _noInputTime = 0.0f;
	float _latchedTargetYaw = 0.0f;
	bool _latched = false;
	Unigine::Math::Vec3 _velDirSmoothed = Unigine::Math::Vec3_forward;
	float _currentMaxRate = 0.0f;
};
