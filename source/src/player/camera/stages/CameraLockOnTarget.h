#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraLockOnTarget final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraLockOnTarget, CameraStageModifier);

	PROP_PARAM(Toggle, reset_mouse_input, false)
	PROP_PARAM(Float, lock_radius, 100.0f)
	PROP_PARAM(Vec2, pitch_limit_deg, {-20.0f, 30.0f})
	PROP_PARAM(Vec2, camera_offset, {0.2f, -0.3f})
	PROP_PARAM(Float, transition_speed, 6.0f)
	PROP_PARAM(Float, retarget_delay, 0.3f)
	PROP_PARAM(Float, recovery_delay, 4.0f)
	PROP_PARAM(Float, lost_los_timer, 3.0f)
	PROP_PARAM(Float, max_deg_per_sec, 720.0f)
	PROP_PARAM(Mask, intersection_mask, (int)0xffffffff)

	void runtimeReset(CameraState &, const CameraContext &) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

	void requestToggle() { pendingToggle = true; }
	void requestRetarget(int dir) { pendingRetargetDir = dir; }

	bool isLocked() const { return locked; }
	Unigine::NodePtr getLockedTarget() const { return lockedTarget; }

private:
	Unigine::NodePtr findClosestTargetable(const Unigine::Math::Vec3 &from, float radius) const;
	bool hasLineOfSight(const Unigine::Math::Vec3 &from, const Unigine::Math::Vec3 &to, const Unigine::NodePtr &target) const;

	static float expAlpha(float k, float dt) { return 1.0f - Unigine::Math::exp(-k * dt); }

	static void screenOffsetToAngleOffset(
		float vfovDeg,
		float aspect,
		const Unigine::Math::Vec2 &screenOffset,
		float &yawOffsetDeg,
		float &pitchOffsetDeg);

private:
	Unigine::NodePtr lockedTarget;
	bool locked = false;

	bool pendingToggle = false;
	int pendingRetargetDir = 0;

	float retargetCooldown = 0.0f;
	float lostLoSTime = 0.0f;
	float noInputTime = 0.0f;

	bool latched = false;
	float yawLatched = 0.0f;
	float pitchLatched = 0.0f;

	Unigine::Math::vec2 prevRigAngle = Unigine::Math::vec2_zero;
	bool prevAngleValid = false;
};
