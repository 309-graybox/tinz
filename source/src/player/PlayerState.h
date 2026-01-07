#pragma once
#include <UnigineObjects.h>
#include <UnigineGame.h>

struct PlayerInput
{
	bool forward;
	bool backward;
	bool left;
	bool right;
	bool shift;
	bool space;
};

class PlayerContext
{
public:
	const PlayerInput &getInput() const noexcept { return _input; }
	void setInput(PlayerInput input) noexcept { _input = std::move(input); }

	const Unigine::ObjectMeshSkinnedPtr &getMeshSkinned() const noexcept { return _meshSkinned; }
	void setMeshSkinned(Unigine::ObjectMeshSkinnedPtr meshSkinned) noexcept { _meshSkinned = std::move(meshSkinned); }

	void setProceduralLayer(int layer) { _proceduralLayer = layer; }
	void setLayer(int layer)
	{
		if (_layer != layer)
		{
			_weight = 0;
			_layer0 = _layer;
		}
		_layer = layer;
	}

	void update()
	{
		for (int i = 0; i < _meshSkinned->getNumLayers(); ++i)
			_meshSkinned->setLayerFrame(0, Unigine::Game::getTime() * 30.0f);

		_weight = Unigine::Math::clamp(_weight + Unigine::Game::getIFps(), 0.0f, 1.0f);

		_meshSkinned->lerpLayer(_proceduralLayer, _layer0, _layer, _weight);
		// Unigine::Log::error("------ Lerp %i %i %i %f\n", _proceduralLayer, _layer0, _layer, _weight);
	}

private:
	PlayerInput _input;
	Unigine::ObjectMeshSkinnedPtr _meshSkinned;
	float _weight = 0.0f;
	int _proceduralLayer = 0;
	int _layer = 0;
	int _layer0 = 0;
};

class PlayerState
{
public:
	virtual ~PlayerState();

	virtual const char *getStateName() const = 0;

	int getAnimationLayer() const noexcept { return _animationLayer; }
	void setAnimationLayer(int layer) noexcept { _animationLayer = layer; }

	void onInit(PlayerContext &ctx);
	void onEnter(PlayerContext &ctx);
	void onUpdate(PlayerContext &ctx);
	void onExit(PlayerContext &ctx);

	PlayerState *getParent() noexcept;
	const PlayerState *getParent() const noexcept;
	void setParent(PlayerState *parent);

	void addChild(PlayerState *child);
	void removeChild(PlayerState *child);

	const Unigine::Vector<PlayerState *> &getChildren() const noexcept;

protected:
	virtual void onInitImpl(PlayerContext &ctx) {}
	virtual void onEnterImpl(PlayerContext &ctx) {}
	virtual void onUpdateImpl(PlayerContext &ctx) {}
	virtual void onExitImpl(PlayerContext &ctx) {}

private:
	PlayerState *_parent;
	Unigine::Vector<PlayerState *> _children;

	int _animationLayer = -1;
};
