#pragma once

#include "MenuButton.h"


class MenuButtonExit : public MenuButton
{
public:
	COMPONENT_DEFINE(MenuButtonExit, MenuButton);

	void onClick() override;
};

