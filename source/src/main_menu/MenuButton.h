#pragma once

#include "MenuInteractive.h"


class MenuButton : public MenuInteractive
{
public:
	COMPONENT_DEFINE(MenuButton, MenuInteractive);

	PROP_GROUP("Hover")
	PROP_PARAM(Vec3, hoverOffset)
	PROP_PARAM(Float, easeSpeed, 8.0f, "Ease Speed")

	PROP_GROUP("Click")
	PROP_PARAM(Vec3, clickOffset)
	PROP_PARAM(Float, clickDelay, 0.4f, "Click Delay", "Seconds between press and action")
	PROP_PARAM(Toggle, fadeOnClick, 0, "Fade On Click", "Ramp screen brightness during click delay")
	PROP_PARAM(String, clickSound, "", "Click Sound")

	bool isPressed() const noexcept { return _pressed; }
	float getClickDelay() const noexcept { return clickDelay; }
	bool shouldFadeOnClick() const noexcept { return (int)fadeOnClick != 0; }

	void press();

	virtual void onClick() {}

protected:
	void onUpdate() override;

private:
	bool _pressed = false;
};

