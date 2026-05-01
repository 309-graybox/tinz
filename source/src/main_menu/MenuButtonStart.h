#pragma once

#include "MenuButton.h"


class MenuButtonStart : public MenuButton
{
public:
	COMPONENT_DEFINE(MenuButtonStart, MenuButton);

	PROP_GROUP("Start")
	PROP_PARAM(File, worldFile, "", "World", "World to load on click")

	void onClick() override;
};

