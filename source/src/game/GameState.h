#pragma once
#include <UnigineEvent.h>
#include <UnigineMathLib.h>
#include <UnigineNode.h>

namespace game
{

enum class Difficulty : int
{
	Easy = 0,
	Normal = 1,
	Hard = 2,
};

// Process-wide game state. Lives across worlds, owned by AppWorldLogic.
// Future home for run-wide stats: score, kills, modifiers, etc.
class GameState
{
public:
	static void init();
	static void shutdown();

	// Tick once per frame from AppWorldLogic::update — drives the death/respawn
	// timer. Polls the player Entity for the alive→dead transition; the player
	// Entity must have persistOnDeath=true so the node isn't auto-deleted.
	static void update();

	static Difficulty getDifficulty();
	static void setDifficulty(Difficulty d);

	// Player character node — the body PlayerCameraManager tracks via target_node,
	// not the camera itself. Resolved lazily on first call and cached.
	// Null in contexts without a player (e.g. main menu).
	static Unigine::NodePtr getPlayerCharacter();

	// Drop the cached character — call when switching worlds or replacing the
	// character at runtime so the next getPlayerCharacter() re-resolves.
	static void invalidatePlayerCharacter();

	// --- Death & respawn ---------------------------------------------------
	// Last checkpoint position; if not set, the player respawns at their initial
	// world position (captured the first time getPlayerCharacter() resolves).
	static void setLastCheckpoint(const Unigine::Math::Vec3 &pos);
	static Unigine::Math::Vec3 getLastCheckpoint();
	static bool hasCheckpoint();

	// Time (seconds) between hp→0 and respawn. Lets the death anim/UI play.
	static void setRespawnDelay(float seconds);
	static float getRespawnDelay();

	// Force an immediate respawn. Resets HP, teleports, fires eventPlayerRespawned.
	static void respawnPlayer();

	// True between hp→0 and respawn. Other systems can gate input/AI/etc. on this.
	static bool isPlayerDead();

	// Fires once on alive→dead transition; eventPlayerRespawned fires when the
	// player is back in control after the death timer.
	static Unigine::EventInvoker<> &eventPlayerDied();
	static Unigine::EventInvoker<> &eventPlayerRespawned();
};

} // namespace game
