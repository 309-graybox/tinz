#include "MenuVolumeSlider.h"
#include "audio/SoundManager.h"

#include <UnigineSounds.h>

REGISTER_COMPONENT(MenuVolumeSlider);

void MenuVolumeSlider::onValueChanged(float v01)
{
	audio::SoundManager::setMasterVolume(v01);
}
