// Copyright (C), UNIGINE. All rights reserved.
#include "ImGuiManipulator.h"

#include "../imgui/ImGuiImpl.h"

#include <UnigineGame.h>
#include <UnigineGui.h>
#include <UniginePlayers.h>
#include <UnigineWidgets.h>

using namespace Unigine;
using namespace Math;

struct Manipulators
{
	WidgetManipulatorTranslatorPtr translate;
	WidgetManipulatorRotatorPtr rotate;
	WidgetManipulatorScalerPtr scale;

	Mat4 transform;
};
static Vector<Manipulators> manipulators;

struct ManipulatorsFrameData
{
	GuiPtr gui;
	CameraPtr camera;
	Mat4 modelview;
	mat4 projection;

	int num_manipulators = 0;
	bool changed = false;
};
static ManipulatorsFrameData manipulators_data;

void ImGuiManipulator::begin()
{
	ManipulatorsFrameData &data = manipulators_data;
	data.gui = Gui::getCurrent();
	data.camera = Game::getPlayer() ? Game::getPlayer()->getCamera() : Camera::create();
	data.modelview = manipulators_data.camera->getModelview();
	data.projection = manipulators_data.camera->getProjection();
	data.num_manipulators = 0;
	data.changed = false;
}

bool ImGuiManipulator::show(Mat4 &transform, int type, const Mat4 &basis)
{
	bool changed = false;
	ManipulatorsFrameData &data = manipulators_data;

	// create new manipulator
	if (manipulators.size() <= data.num_manipulators)
	{
		Manipulators &m = manipulators.append();
		m.transform = transform;

		WidgetManipulatorTranslatorPtr &t = m.translate;
		t = WidgetManipulatorTranslator::create(data.gui);
		if (type != Widget::WIDGET_MANIPULATOR_TRANSLATOR)
			t->setHidden(true);
		t->setBasis(basis);
		t->setModelview(data.modelview);
		t->setProjection(data.projection);
		t->setTransform(transform);
		t->setRenderGui(data.gui);
		t->setSize(96);
		data.gui->addChild(t, Gui::ALIGN_OVERLAP);

		WidgetManipulatorRotatorPtr &r = m.rotate;
		r = WidgetManipulatorRotator::create(data.gui);
		if (type != Widget::WIDGET_MANIPULATOR_ROTATOR)
			r->setHidden(true);
		r->setBasis(basis);
		r->setModelview(data.modelview);
		r->setProjection(data.projection);
		r->setTransform(transform);
		r->setRenderGui(data.gui);
		r->setSize(96);
		data.gui->addChild(r, Gui::ALIGN_OVERLAP);

		WidgetManipulatorScalerPtr &s = m.scale;
		s = WidgetManipulatorScaler::create(data.gui);
		if (type != Widget::WIDGET_MANIPULATOR_SCALER)
			s->setHidden(true);
		s->setBasis(basis);
		s->setModelview(data.modelview);
		s->setProjection(data.projection);
		s->setTransform(transform);
		s->setRenderGui(data.gui);
		s->setSize(96);
		data.gui->addChild(s, Gui::ALIGN_OVERLAP);
	}
	// change current manipulator
	else
	{
		Manipulators &m = manipulators[data.num_manipulators];

		bool type_changed = false;
		if (type == Widget::WIDGET_MANIPULATOR_TRANSLATOR && m.translate->isHidden())
		{
			type_changed = true;
			m.translate->setHidden(false);
			m.rotate->setHidden(true);
			m.scale->setHidden(true);
		}
		else if (type == Widget::WIDGET_MANIPULATOR_ROTATOR && m.rotate->isHidden())
		{
			type_changed = true;
			m.translate->setHidden(true);
			m.rotate->setHidden(false);
			m.scale->setHidden(true);
		}
		else if (type == Widget::WIDGET_MANIPULATOR_SCALER && m.scale->isHidden())
		{
			type_changed = true;
			m.translate->setHidden(true);
			m.rotate->setHidden(true);
			m.scale->setHidden(false);
		}

		WidgetManipulatorPtr manipulator = m.translate;
		if (type == Widget::WIDGET_MANIPULATOR_ROTATOR)
			manipulator = m.rotate;
		else if (type == Widget::WIDGET_MANIPULATOR_SCALER)
			manipulator = m.scale;
		manipulator->setBasis(basis);
		manipulator->setModelview(data.modelview);
		manipulator->setProjection(data.projection);

		// update position of the manipulator
		if (transform != manipulator->getTransform())
		{
			// check (user has been moving this manipulator?)
			if (!type_changed && manipulator->isFocusAxis()
				&& Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT)
				&& !ImGuiImpl::isWantCaptureMouse())
			{
				changed = true;
				m.transform = manipulator->getTransform();
				transform = m.transform;	// return to user changed result
				data.changed = true;
			}
			else
			{
				manipulator->setTransform(transform);
				m.transform = transform;
			}
		}
	}

	data.num_manipulators++;
	return changed;
}

void ImGuiManipulator::end()
{
	// hide unused manipulators
	for (int i = manipulators_data.num_manipulators; i < manipulators.size(); i++)
	{
		Manipulators &m = manipulators[i];
		m.translate->setHidden(true);
		m.rotate->setHidden(true);
		m.scale->setHidden(true);
	}
}

bool ImGuiManipulator::isHovered()
{
	for (int i = 0; i < manipulators_data.num_manipulators; i++)
	{
		Manipulators &m = manipulators[i];
		if (!m.translate->isHidden() && m.translate->isHoverAxis())
			return true;
		if (!m.rotate->isHidden() && m.rotate->isHoverAxis())
			return true;
		if (!m.scale->isHidden() && m.scale->isHoverAxis())
			return true;
	}
	return false;
}

void ImGuiManipulator::destroy()
{
	// destroy widget manipulators
	for (int i = 0; i < manipulators.size(); i++)
	{
		Manipulators &m = manipulators[i];
		m.translate.deleteLater();
		m.rotate.deleteLater();
		m.scale.deleteLater();
	}
	manipulators.clear();
	manipulators_data.num_manipulators = 0;
}
