#pragma once

#include "MenuInteractive.h"

#include <UnigineMathLib.h>
#include <UnigineNode.h>


// One configurable hover-rotation entry: target node swings around `pivot`
// (or its own origin if `pivot` is null) by `angle` degrees on hover.
// `speed` drives interpolation toward the open pose; `damping` toward the
// rest pose.
struct HoverPivotAnim : public Unigine::ComponentStruct
{
	PROP_PARAM(Node, target, "Target")
	PROP_PARAM(Node, pivot, "Pivot", "Optional rotation pivot (null = target's own origin)")
	PROP_PARAM(Vec3, axis, Unigine::Math::vec3(0.0f, 0.0f, 1.0f), "Axis", "Rotation axis in pivot's local space")
	PROP_PARAM(Float, angle, 0.0f, "Angle", "Hover rotation, degrees (0 = no rotation)")
	PROP_PARAM(Vec3, offset)
	PROP_PARAM(Float, speed, 8.0f, "Speed", "Approach rate toward open pose")
	PROP_PARAM(Float, damping, 8.0f, "Damping", "Return rate to rest pose")
};


class MenuButton : public MenuInteractive
{
public:
	COMPONENT_DEFINE(MenuButton, MenuInteractive);

	PROP_GROUP("Hover")
	PROP_ARRAY_STRUCT(HoverPivotAnim, hoverAnims)
	PROP_ARRAY(Node, hoverToggleNodes)

	PROP_GROUP("Click")
	PROP_PARAM(Float, clickDelay, 0.4f, "Click Delay", "Seconds between press and action")
	PROP_PARAM(Toggle, fadeOnClick, 0, "Fade On Click", "Ramp screen brightness during click delay")
	PROP_PARAM(String, clickSound, "", "Click Sound")

	bool isPressed() const noexcept { return _pressed; }
	float getClickDelay() const noexcept { return clickDelay; }
	bool shouldFadeOnClick() const noexcept { return (int)fadeOnClick != 0; }

	void press();

	virtual void onClick() {}

	void setHovered(bool on) override;

protected:
	void onInit() override;
	void onUpdate() override;

private:
	struct AnimState
	{
		Unigine::Math::Vec3 rest_pos = Unigine::Math::Vec3_zero;
		Unigine::Math::quat rest_rot;
		float t = 0.0f;
	};

	Unigine::Vector<AnimState> _anim_states;
	bool _pressed = false;
};

