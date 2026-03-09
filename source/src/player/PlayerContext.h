#pragma once

#include <UnigineObjects.h>
#include <UnigineGame.h>

class PlayerContext
{
public:
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
			_meshSkinned->setLayerFrame(i, Unigine::Game::getTime() * 30.0f);

		_weight = Unigine::Math::clamp(_weight + Unigine::Game::getIFps(), 0.0f, 1.0f);

		_meshSkinned->lerpLayer(_proceduralLayer, _layer0, _layer, _weight);
	}

private:
	Unigine::ObjectMeshSkinnedPtr _meshSkinned;
	float _weight = 0.0f;
	int _proceduralLayer = 0;
	int _layer = 0;
	int _layer0 = 0;
};
