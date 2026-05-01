#include "MenuVolumeSlider.h"

#include <UnigineSounds.h>

REGISTER_COMPONENT(MenuVolumeSlider);

void MenuVolumeSlider::onValueChanged(float v01)
{
	Unigine::Sound::setVolume(v01);
}
