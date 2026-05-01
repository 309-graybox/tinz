#pragma once
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

	static Difficulty getDifficulty();
	static void setDifficulty(Difficulty d);

	// Player character node — the body PlayerCameraManager tracks via target_node,
	// not the camera itself. Resolved lazily on first call and cached.
	// Null in contexts without a player (e.g. main menu).
	static Unigine::NodePtr getPlayerCharacter();

	// Drop the cached character — call when switching worlds or replacing the
	// character at runtime so the next getPlayerCharacter() re-resolves.
	static void invalidatePlayerCharacter();
};

} // namespace game
