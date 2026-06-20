#include "SurfaceRegistry.h"

REGISTER_COMPONENT(SurfaceRegistry)

using namespace Unigine;

SurfaceRegistry *SurfaceRegistry::_instance = nullptr;

void SurfaceRegistry::init()
{
	if (_instance && _instance != this)
		Log::warning("SurfaceRegistry: multiple instances present, latest wins\n");
	_instance = this;
}

void SurfaceRegistry::shutdown()
{
	if (_instance == this)
		_instance = nullptr;
}

SurfaceConfig *SurfaceRegistry::find(const char *id)
{
	if (!_instance || !id || !*id)
		return nullptr;

	auto &arr = _instance->surfaces;
	for (int i = 0; i < arr.size(); ++i)
	{
		SurfaceConfig &c = arr[i].get();
		if (strcmp(c.id.get(), id) == 0)
			return &c;
	}
	return nullptr;
}
