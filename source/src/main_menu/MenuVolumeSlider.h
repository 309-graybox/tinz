#pragma once

#include "MenuDragger.h"


class MenuVolumeSlider : public MenuDragger
{
public:
	COMPONENT_DEFINE(MenuVolumeSlider, MenuDragger);

	void onValueChanged(float v01) override;
};

