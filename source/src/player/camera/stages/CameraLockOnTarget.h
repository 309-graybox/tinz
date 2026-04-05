#pragma once
#include "player/camera/CameraStageModifier.h"

class CameraLockOnTarget final: public CameraStageModifier
{
public:
	COMPONENT_DEFINE(CameraLockOnTarget, CameraStageModifier);

	// target acquisition
	PROP_PARAM(Float, lock_radius, 50.0f)
	PROP_PARAM(Float, retarget_delay, 0.3f)
	PROP_PARAM(Mask, intersection_mask, (int)0xffffffff)
	PROP_PARAM(Float, lost_los_timer, 3.0f)

	// transition blend
	PROP_PARAM(Float, transition_speed, 6.0f)

	// locked-camera geometry
	PROP_PARAM(Float, extra_arm_offset, 1.0f)
	PROP_PARAM(Vec2, arm_clamp, {3.0f, 200.0f})
	PROP_PARAM(Vec2, pitch_limit_deg, {-20.0f, 30.0f})

public:
	void runtimeReset(CameraState &, const CameraContext &) override;
	void apply(CameraState &state, const CameraInput &input, const CameraContext &ctx) override;

	void requestRetarget(int dir) { pendingRetargetDir = dir; }

	bool isLocked() const { return lockedTarget.isValid(); }
	Unigine::NodePtr getLockedTarget() const { return lockedTarget; }

private:
	void drawDebug();
	void updateBlendWeight(float dt);

	void processRetarget(const CameraState &state, const CameraContext &ctx);
	void validateTarget(const CameraState &state, const CameraContext &ctx);
	void updateLock(const CameraState &state, const CameraContext &ctx);
	void applyToRig(CameraState &state);

	Unigine::NodePtr findClosestTargetable(const Unigine::Math::Vec3 &from, float radius) const;
	Unigine::NodePtr findRetarget(const Unigine::Math::Vec3 &from, float radius, int dir, const Unigine::Math::vec2 &camAngle) const;
	bool hasLoS(const Unigine::Math::Vec3 &from, const Unigine::Math::Vec3 &to, const Unigine::NodePtr &ignore) const;

	static float expAlpha(float k, float dt) { return 1.0f - Unigine::Math::exp(-k * dt); }

private:
	Unigine::NodePtr lockedTarget;

	int pendingRetargetDir = 0;
	float retargetCooldown = 0.0f;

	float lostLoSTime = 0.0f;
	float blendWeight = 0.0f;

	// cached locked-state values (kept valid during blend-out)
	Unigine::Math::Vec3 cachedPivot;
	double cachedArm = 3.0;
	bool cacheValid = false;
};
