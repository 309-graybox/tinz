#include "CameraLockOnTarget.h"
#include "tuning/DebugTuning.h"
#include "components/enemies/Targetable.h"
#include <UniginePlayers.h>
#include <UnigineWindowManager.h>
#include <UnigineVisualizer.h>

REGISTER_COMPONENT(CameraLockOnTarget)

using namespace Unigine;
using namespace Unigine::Math;

void CameraLockOnTarget::runtimeReset(CameraState &, const CameraContext &)
{
	lockedTarget = nullptr;

	pendingRetargetDir = 0;
	retargetCooldown = 0.0f;

	lostLoSTime = 0.0f;
	blendWeight = 0.0f;

	cachedPivot = Vec3(0.0);
	cachedArm = 3.0;
	cacheValid = false;

	targetAnchor = Vec3(0.0);
	anchorValid = false;
}

void CameraLockOnTarget::apply(CameraState &state, const CameraInput &input, const CameraContext &ctx)
{
	if (!ctx.target)
		return;

	drawDebug();

	Vec3 ownerPos = ctx.targetPosition;

	// --- timers ---
	retargetCooldown = max(0.0f, retargetCooldown - state.dt);

	// --- toggle ---
	if (input.targetLock)
	{
		lockedTarget = lockedTarget ? nullptr : findClosestTargetable(ownerPos, lock_radius.get());
		anchorValid = false;
	}

	processRetarget(state, ctx);
	validateTarget(state, ctx);

	if (!lockedTarget)
		lostLoSTime = 0.0f;

	updateBlendWeight(state.dt);
	updateLock(state, ctx);

	if (!cacheValid)
		return;

	applyToRig(state);
}

void CameraLockOnTarget::drawDebug()
{
	if (DebugTuning::get()->show_camera_lock_on_direction && lockedTarget)
		Visualizer::renderPoint3D(lockedTarget->getWorldPosition(), 0.1f, vec4_red, true, 0.0f, false);
}

void CameraLockOnTarget::updateBlendWeight(float dt)
{
	float targetW = lockedTarget ? 1.0f : 0.0f;
	float alpha = expAlpha(transition_speed.get(), dt);
	blendWeight += (targetW - blendWeight) * alpha;

	if (blendWeight < 1e-3f && targetW < 1e-3f)
	{
		blendWeight = 0.0f;
		cacheValid = false;
		return;
	}

	if (blendWeight > 1.0f - 1e-3f && targetW > 1.0f - 1e-3f)
		blendWeight = 1.0f;
}

void CameraLockOnTarget::processRetarget(const CameraState &state, const CameraContext &ctx)
{
	if (!lockedTarget || pendingRetargetDir == 0 || retargetCooldown > 0.0f)
		return;

	NodePtr next = findRetarget(ctx.targetPosition, lock_radius.get(), pendingRetargetDir, state.rig.angle);
	if (next)
	{
		lockedTarget = next;
		retargetCooldown = retarget_delay.get();
		anchorValid = false;
	}

	pendingRetargetDir = 0;
}

void CameraLockOnTarget::validateTarget(const CameraState &state, const CameraContext &ctx)
{
	if (!lockedTarget)
		return;

	float maxR = lock_radius.get();
	Vec3 enemyPos = lockedTarget->getWorldPosition();

	// range
	if (length2(enemyPos - ctx.targetPosition) > maxR * maxR)
	{
		lockedTarget = nullptr;
		return;
	}

	// LOS
	Vec3 camPos = ctx.cameraNode ? ctx.cameraNode->getWorldPosition() : state.pos;
	bool los = hasLoS(camPos, enemyPos, lockedTarget);
	lostLoSTime = los ? 0.0f : lostLoSTime + state.dt;

	if (lostLoSTime > lost_los_timer.get())
	{
		lockedTarget = nullptr;
	}
}

void CameraLockOnTarget::updateLock(const CameraState &state, const CameraContext &ctx)
{
	if (!lockedTarget)
		return;

	Vec3 ownerPos = ctx.targetPosition;
	Vec3 enemyPos = lockedTarget->getWorldPosition();

	// deadzone: anchor stays put while target is within radius
	if (!anchorValid)
	{
		targetAnchor = enemyPos;
		anchorValid = true;
	}

	bool anchorFrozen;
	if (screen_deadzone.get())
	{
		// screen-space mode: freeze anchor while target is within screen margins
		anchorFrozen = isOnScreen(enemyPos, ctx);
	} else
	{
		// world-space mode: freeze anchor while target is within radius
		float anchorDist = (float)length(enemyPos - targetAnchor);
		anchorFrozen = anchorDist <= target_deadzone.get();
	}

	if (!anchorFrozen)
	{
		Vec3 anchorDelta = enemyPos - targetAnchor;
		float anchorDist = (float)length(anchorDelta);
		if (anchorDist > 1e-4f)
		{
			Vec3 dir = anchorDelta / anchorDist;
			float step = anchorDist * expAlpha(target_follow_speed.get(), state.dt);
			targetAnchor = targetAnchor + dir * step;
		}
	}

	cachedPivot = (ownerPos + targetAnchor) * 0.5;

	float halfDist = (float)length(targetAnchor - ownerPos) * 0.5f;
	float halfFovRad = state.fov * 0.5f * Consts::DEG2RAD;
	float requiredArm = halfDist / max(tan(halfFovRad), 1e-4f);
	cachedArm = clamp(requiredArm + extra_arm_offset.get(), arm_clamp.get().x, arm_clamp.get().y);

	cacheValid = true;
}

void CameraLockOnTarget::applyToRig(CameraState &state)
{
	state.rig.pivot = Math::lerp(state.rig.pivot, cachedPivot, blendWeight);
	state.rig.arm_len = Math::lerp(state.rig.arm_len, cachedArm, (double)blendWeight);

	float clampedPitch = clamp(state.rig.angle.y, pitch_limit_deg.get().x, pitch_limit_deg.get().y);
	state.rig.angle.y = Math::lerp(state.rig.angle.y, clampedPitch, blendWeight);
}

NodePtr CameraLockOnTarget::findClosestTargetable(const Vec3 &from, float radius) const
{
	float bestDist2 = radius * radius;
	NodePtr best;

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

		auto p = node->getWorldPosition();
		auto d2 = length2(p - from);
		if (d2 < bestDist2)
		{
			bestDist2 = d2;
			best = node;
		}
	}
	return best;
}

NodePtr CameraLockOnTarget::findRetarget(const Vec3 &from, float radius, int dir, const vec2 &camAngle) const
{
	float yawRad = camAngle.x * Consts::DEG2RAD;
	Vec3 camRight = Vec3(cos(yawRad), -sin(yawRad), 0.0);

	float bestScore = Consts::INF;
	NodePtr best;

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
		if (node == lockedTarget)
			continue;

		Vec3 toNode = node->getWorldPosition() - from;
		float lateral = dot(toNode, camRight) * dir;
		if (lateral <= 0.0f)
			continue;

		float dist = length(toNode);
		float score = dist / (lateral + 1.0f);

		if (score < bestScore)
		{
			bestScore = score;
			best = node;
		}
	}
	return best;
}

bool CameraLockOnTarget::isOnScreen(const Vec3 &worldPos, const CameraContext &ctx) const
{
	if (!ctx.cameraNode)
		return false;

	PlayerPtr player = checked_ptr_cast<Player>(ctx.cameraNode);
	if (!player)
		return false;

	int px, py;
	int visible = player->getMainWindowPosition(px, py, worldPos);
	if (!visible)
		return false;

	ivec2 winSize = WindowManager::getMainWindow()->getClientSize();
	if (winSize.x <= 0 || winSize.y <= 0)
		return false;

	vec2 m = screen_margin.get();
	float nx = (float)px / (float)winSize.x;
	float ny = (float)py / (float)winSize.y;

	return nx >= m.x && nx <= (1.0f - m.x) && ny >= m.y && ny <= (1.0f - m.y);
}

bool CameraLockOnTarget::hasLoS(const Vec3 &from, const Vec3 &to, const NodePtr &ignore) const
{
	if (length(to - from) < 1e-3f)
		return true;

	auto hit = static_ptr_cast<Node>(World::getIntersection(from, to, intersection_mask));
	while (hit)
	{
		if (hit == ignore)
			return true;
		hit = hit->getParent();
	}
	return false;
}