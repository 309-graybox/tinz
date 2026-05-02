#pragma once

#include "MenuInteractive.h"

#include <UnigineImage.h>


// Diegetic 1D slider — the user grabs the assembly with LMB and drags it
// vertically along a configured axis. Movement maps to a 0..1 value.
//
// Convention: editor places the node in the "fully shown" pose (value=1).
// `buryOffset` is the vector from that pose to the "fully buried/min" pose
// (value=0). E.g. buryOffset = (0, 0, -0.5) buries the node 50cm down.
class MenuDragger : public MenuInteractive
{
public:
	COMPONENT_DEFINE(MenuDragger, MenuInteractive);

	PROP_GROUP("Drag")
	PROP_PARAM(File, hoverCursor)
	PROP_PARAM(File, dragCursor)
	PROP_PARAM(Vec3, buryOffset)
	PROP_PARAM(Float, sensitivity, 200.0f, "Sensitivity", "Pixels of vertical mouse movement = full 0..1 range")
	PROP_PARAM(Float, initialValue, 1.0f, "Initial Value", "Starting slider value 0..1")
	PROP_PARAM(Float, easeSpeed, 12.0f, "Ease Speed")

	bool isDragging() const noexcept { return _dragging; }
	float getValue() const noexcept { return _value; }

	void setHovered(bool on, bool play_sound = true) override;
	void beginDrag(int mouse_y);
	void updateDrag(int mouse_y);
	void endDrag();

	virtual void onValueChanged(float v01) {}

protected:
	void onInit() override;
	void onUpdate() override;

	void apply_value(float v01);

	float _value = 1.0f;

private:
	void apply_cursor();

	float _drag_start_value = 1.0f;
	int _drag_start_mouse_y = 0;
	bool _dragging = false;

	Unigine::ImagePtr _hover_cursor_image;
	Unigine::ImagePtr _drag_cursor_image;
};

