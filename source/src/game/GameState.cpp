#include "GameState.h"
#include "player/camera/PlayerCameraManager.h"

#include <UnigineComponentSystem.h>
#include <UnigineGame.h>

namespace game
{

using namespace Unigine;

namespace
{

struct State
{
	bool initialized = false;
	Difficulty difficulty = Difficulty::Normal;
	NodePtr player_character;
};

State &state()
{
	static State s;
	return s;
}

} // namespace

void GameState::init()
{
	State &s = state();
	s.player_character.clear();
	s.difficulty = Difficulty::Normal;
	s.initialized = true;
}

void GameState::shutdown()
{
	State &s = state();
	s.player_character.clear();
	s.initialized = false;
}

Difficulty GameState::getDifficulty()
{
	return state().difficulty;
}

void GameState::setDifficulty(Difficulty d)
{
	state().difficulty = d;
}

NodePtr GameState::getPlayerCharacter()
{
	State &s = state();
	if (s.player_character)
		return s.player_character;

	auto player = Game::getPlayer();
	if (!player)
		return {};

	auto pcm = ComponentSystem::get()->getComponent<PlayerCameraManager>(static_ptr_cast<Node>(player));
	if (!pcm)
		return {};

	s.player_character = pcm->target_node.get();
	return s.player_character;
}

void GameState::invalidatePlayerCharacter()
{
	state().player_character.clear();
}

} // namespace game
