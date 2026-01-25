#include "CameraLockOnTarget.h"
#include "components/Targetable.h"
#include <UnigineVisualizer.h>

REGISTER_COMPONENT(CameraLockOnTarget)

using namespace Unigine;
using namespace Unigine::Math;

void CameraLockOnTarget::runtimeReset(CameraState &, const CameraContext &)
{
	lockedTarget = nullptr;
	locked = false;

	pendingToggle = false;
	pendingRetargetDir = 0;

	retargetCooldown = 0.0f;
	lostLoSTime = 0.0f;
	noInputTime = 0.0f;

	latched = false;
	yawLatched = 0.0f;
	pitchLatched = 0.0f;

	prevRigAngle = vec2_zero;
	prevAngleValid = false;
}

void CameraLockOnTarget::screenOffsetToAngleOffset(
	float vfovDeg,
	float aspect,
	const Vec2 &screenOffset,
	float &yawOffsetDeg,
	float &pitchOffsetDeg)
{
	const float vfov = vfovDeg * Consts::DEG2RAD;
	const float hfov = 2.0f * atan(tan(vfov * 0.5f) * aspect);

	yawOffsetDeg = atan(screenOffset.x * tan(hfov * 0.5f)) * Consts::RAD2DEG;
	pitchOffsetDeg = atan(screenOffset.y * tan(vfov * 0.5f)) * Consts::RAD2DEG;
}

NodePtr CameraLockOnTarget::findClosestTargetable(const Vec3 &from, float radius) const
{
	float bestDist2 = radius * radius;
	NodePtr best = nullptr;

	Vector<ObjectPtr> objects;
	WorldBoundSphere bs(from, radius);
	World::getIntersection(bs, objects);

	for (auto &o : objects)
	{
		Targetable *t = getComponentInChildren<Targetable>(o);
		if (!t)
			continue;

		NodePtr node = t->getTarget(from);
		if (!node || !node->isEnabled())
			continue;

		auto d2 = length2(node->getWorldPosition() - from);
		if (d2 < bestDist2)
		{
			bestDist2 = d2;
			best = node;
		}
	}
	return best;
}

bool CameraLockOnTarget::hasLineOfSight(const Vec3 &from, const Vec3 &to, const NodePtr &target) const
{
	Vec3 dir = to - from;
	float dist = length(dir);
	if (dist < 1e-3f)
		return true;

	dir /= dist;

	auto o = static_ptr_cast<Node>(World::getIntersection(from, to, intersection_mask));
	if (o)
	{
		while (o)
		{
			if (o == target)
				return true;

			o = o->getParent();
		}
	}

	return false;
}

void CameraLockOnTarget::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	if (debug && lockedTarget)
	{
		Visualizer::renderPoint3D(lockedTarget->getWorldPosition(), 0.1f, vec4_red, true, 0.0f, false);
	}

	if (!ctx.target)
		return;

	// capture last stable angle when not locked
	if (!locked)
	{
		prevRigAngle = state.rig.angle;
		prevAngleValid = true;
	}

	if (input.targetLock)
	{
		pendingToggle = true;
	}

	// --- timers ---
	retargetCooldown = max(0.0f, retargetCooldown - state.dt);

	const bool hasInput =
		abs(input.angle.x) > 1e-4f ||
		abs(input.angle.y) > 1e-4f ||
		abs(input.scroll) > 1e-6f;

	noInputTime = hasInput ? 0.0f : noInputTime + state.dt;

	// --- toggle ---
	if (pendingToggle)
	{
		pendingToggle = false;

		if (locked)
		{
			locked = false;
			lockedTarget = nullptr;
			latched = false;
			return;
		}

		const Vec3 ownerPos = ctx.target->getWorldPosition();
		lockedTarget = findClosestTargetable(ownerPos, lock_radius.get());
		locked = (lockedTarget != nullptr);
		latched = false;
		return;
	}

	// --- recovery ---
	if (!locked)
	{
		if (noInputTime > recovery_delay.get())
			latched = false;
		return;
	}

	if (!lockedTarget)
	{
		locked = false;
		return;
	}

	// if requested, cancel mouse/orbit influence by restoring previous stable angle
	if (reset_mouse_input.get() && prevAngleValid)
	{
		state.rig.angle = prevRigAngle;
	}

	// --- distance check ---
	const Vec3 ownerPos = ctx.target->getWorldPosition();
	const Vec3 enemyPos = lockedTarget->getWorldPosition();
	const float maxR = lock_radius.get();

	if (length2(enemyPos - ownerPos) > maxR * maxR)
	{
		locked = false;
		lockedTarget = nullptr;
		latched = false;
		return;
	}

	// --- LOS ---
	Vec3 camPos = ctx.cameraNode
					  ? ctx.cameraNode->getWorldPosition()
					  : state.pos;

	const bool los = hasLineOfSight(camPos, enemyPos, lockedTarget);
	lostLoSTime = los ? 0.0f : lostLoSTime + state.dt;

	if (lostLoSTime > lost_los_timer.get())
	{
		locked = false;
		lockedTarget = nullptr;
		latched = false;
		return;
	}

	// --- yaw / pitch to target ---
	Vec3 to = enemyPos - camPos;
	const float h = sqrt(to.x * to.x + to.y * to.y);
	if (h < 1e-4f)
		return;

	Vec3 dirH = normalize(Vec3(to.x, to.y, 0.0f));
	float targetYaw = -atan2(dirH.x, dirH.y) * Consts::RAD2DEG;
	float targetPitch = atan2(to.z, h) * Consts::RAD2DEG;

	// --- composition offset ---
	float yawOffset = 0.0f;
	float pitchOffset = 0.0f;
	screenOffsetToAngleOffset(state.fov, 16.0f / 9.0f, Vec2(camera_offset), yawOffset, pitchOffset);

	targetYaw += yawOffset;
	targetPitch += pitchOffset;

	// --- pitch clamp ---
	auto pr = pitch_limit_deg.get();
	targetPitch = clamp(targetPitch, min(pr.x, pr.y), max(pr.x, pr.y));

	// --- smooth ---
	const float a = expAlpha(transition_speed.get(), state.dt);

	if (!latched)
	{
		yawLatched = targetYaw;
		pitchLatched = targetPitch;
		latched = true;
	} else
	{
		yawLatched += normalizeAngle(targetYaw - yawLatched) * a;
		pitchLatched += normalizeAngle(targetPitch - pitchLatched) * a;
	}

	// --- apply ---
	const float maxStep = max_deg_per_sec.get() * state.dt;

	float yaw = state.rig.angle.x;
	float pitch = state.rig.angle.y;

	yaw += clamp(normalizeAngle(yawLatched - yaw), -maxStep, maxStep);
	pitch += clamp(normalizeAngle(pitchLatched - pitch), -maxStep, maxStep);

	pitch = clamp(pitch, min(pr.x, pr.y), max(pr.x, pr.y));

	state.rig.angle.x = yaw;
	state.rig.angle.y = pitch;

	// update stable angle to the final result (so next frame restore is consistent)
	prevRigAngle = state.rig.angle;
	prevAngleValid = true;
}
