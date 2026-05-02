#include "GameState.h"
#include "components/Entity.h"
#include "player/camera/PlayerCameraManager.h"

#include <UnigineComponentSystem.h>
#include <UnigineGame.h>
#include <UniginePhysics.h>

namespace game
{

using namespace Unigine;
using namespace Unigine::Math;

namespace
{

struct State
{
	bool initialized = false;
	Difficulty difficulty = Difficulty::Normal;
	NodePtr player_character;

	// Death / respawn
	bool player_dead = false;
	float respawn_timer = 0.0f;
	float respawn_delay = 1.5f;

	bool fallback_set = false;
	Vec3 fallback_spawn = Vec3_zero;

	bool checkpoint_set = false;
	Vec3 checkpoint_pos = Vec3_zero;

	EventInvoker<> event_died;
	EventInvoker<> event_respawned;
};

State &state()
{
	static State s;
	return s;
}

void teleport_player_to(const NodePtr &player, const Vec3 &pos)
{
	if (!player)
		return;

	Mat4 t = player->getWorldTransform();
	t.setTranslate(pos);
	player->setWorldTransform(t);

	// Stop any residual velocity on whichever body the character node carries.
	if (auto rigid = player->getObjectBodyRigid())
	{
		rigid->setLinearVelocity(vec3_zero);
		rigid->setAngularVelocity(vec3_zero);
	}
}

} // namespace

void GameState::init()
{
	State &s = state();
	s.player_character.clear();
	s.difficulty = Difficulty::Normal;

	s.player_dead = false;
	s.respawn_timer = 0.0f;
	s.fallback_set = false;
	s.checkpoint_set = false;

	s.initialized = true;
}

void GameState::shutdown()
{
	State &s = state();
	s.player_character.clear();
	s.player_dead = false;
	s.fallback_set = false;
	s.checkpoint_set = false;
	s.initialized = false;
}

void GameState::update()
{
	State &s = state();

	NodePtr player = getPlayerCharacter();
	if (!player)
		return;

	Entity *entity = ComponentSystem::get()->getComponent<Entity>(player);

	// Detect alive→dead transition by polling the player's Entity. The player
	// Entity must have persistOnDeath=true so the node isn't auto-deleted —
	// otherwise getPlayerCharacter() would start returning a stale handle.
	if (entity && entity->isDead() && !s.player_dead)
	{
		s.player_dead = true;
		s.respawn_timer = s.respawn_delay;
		s.event_died.run();
	}

	if (s.player_dead)
	{
		s.respawn_timer -= Game::getIFps();
		if (s.respawn_timer <= 0.0f)
			respawnPlayer();
	}
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

	// First successful resolve — capture the initial position as the fallback
	// spawn for respawns that happen before any checkpoint is recorded.
	if (s.player_character && !s.fallback_set)
	{
		s.fallback_spawn = s.player_character->getWorldPosition();
		s.fallback_set = true;
	}

	return s.player_character;
}

void GameState::invalidatePlayerCharacter()
{
	State &s = state();
	s.player_character.clear();
	s.fallback_set = false;
}

void GameState::setLastCheckpoint(const Vec3 &pos)
{
	State &s = state();
	s.checkpoint_pos = pos;
	s.checkpoint_set = true;
}

Vec3 GameState::getLastCheckpoint()
{
	const State &s = state();
	if (s.checkpoint_set)
		return s.checkpoint_pos;
	if (s.fallback_set)
		return s.fallback_spawn;
	return Vec3_zero;
}

bool GameState::hasCheckpoint()
{
	return state().checkpoint_set;
}

void GameState::setRespawnDelay(float seconds)
{
	state().respawn_delay = max(0.0f, seconds);
}

float GameState::getRespawnDelay()
{
	return state().respawn_delay;
}

void GameState::respawnPlayer()
{
	State &s = state();

	NodePtr player = getPlayerCharacter();
	if (!player)
		return;

	teleport_player_to(player, getLastCheckpoint());

	if (auto entity = ComponentSystem::get()->getComponent<Entity>(player))
		entity->revive();

	s.player_dead = false;
	s.respawn_timer = 0.0f;
	s.event_respawned.run();
}

bool GameState::isPlayerDead()
{
	return state().player_dead;
}

EventInvoker<> &GameState::eventPlayerDied()
{
	return state().event_died;
}

EventInvoker<> &GameState::eventPlayerRespawned()
{
	return state().event_respawned;
}

} // namespace game
