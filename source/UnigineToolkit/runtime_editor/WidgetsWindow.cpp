// Copyright (C), UNIGINE. All rights reserved.

#include "WidgetsWindow.h"

#include "../imgui/imgui/imgui.h"

#include <UnigineWidgets.h>

using namespace Unigine;

namespace {
WidgetPtr widget_selected;

void draw_widgets_list(const WidgetPtr &widget)
{
	if (!widget)
		return;

	StringStack<> name = widget->getTypeName();

	constexpr ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
											  | ImGuiTreeNodeFlags_OpenOnDoubleClick
											  | ImGuiTreeNodeFlags_SpanAvailWidth;
	ImGuiTreeNodeFlags widget_flags = base_flags;
	if (widget_selected == widget.get())
		widget_flags |= ImGuiTreeNodeFlags_Selected;

	if (widget->getNumChildren())
	{
		ImGui::PushStyleColor(ImGuiCol_Text,
			widget->isEnabled() ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		bool node_open =
			ImGui::TreeNodeEx((const void *)widget.get(), widget_flags, "%s", name.get());
		ImGui::PopStyleColor();
		if (ImGui::IsItemClicked())
			widget_selected = widget;
		if (node_open)
		{
			for (int i = 0; i < widget->getNumChildren(); i++)
			{
				draw_widgets_list(widget->getChild(i));
			}
			ImGui::TreePop();
		}
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Text,
			widget->isEnabled() ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::TreeNodeEx((const void *)widget.get(),
			widget_flags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%s",
			name.get());
		ImGui::PopStyleColor();
		if (ImGui::IsItemClicked())
			widget_selected = widget;
	}
}

void draw_widgets_list()
{
	WidgetVBoxPtr vbox = Gui::getCurrent()->getVBox();
	draw_widgets_list(vbox);
}

void draw_widget_parameters()
{
	if (!widget_selected)
		return;

	bool enabled = widget_selected->isEnabled();
	if (ImGui::Checkbox("Enabled", &enabled))
		widget_selected->setEnabled(enabled);

	bool hidden = widget_selected->isHidden();
	if (ImGui::Checkbox("Hidden", &hidden))
		widget_selected->setHidden(hidden);

	bool focused = widget_selected->isFocused() != 0;
	if (ImGui::Checkbox("Focused", &focused) && focused)
		widget_selected->setFocus();

	ImGui::Text("Type: %s", widget_selected->getTypeName());
	ImGui::Text("Data: %s", widget_selected->getData());
	ImGui::Text("Tooltip: %s", widget_selected->getToolTip());
	// TODO setTootip

	int flags = widget_selected->getFlags();
	if (ImGui::InputInt("Flags", &flags, 0, 100, ImGuiInputTextFlags_CharsHexadecimal))
		widget_selected->setFlags(flags);

	int order = widget_selected->getOrder();
	if (ImGui::DragInt("Order", &order))
		widget_selected->setOrder(order);

	Math::ivec2 pos(widget_selected->getPositionX(), widget_selected->getPositionY());
	if (ImGui::DragInt2("Position", pos))
		widget_selected->setPosition(pos.x, pos.y);

	Math::ivec2 size(widget_selected->getWidth(), widget_selected->getHeight());
	if (ImGui::DragInt2("Size", size))
	{
		widget_selected->setWidth(size.x);
		widget_selected->setHeight(size.y);
	}

	Math::ivec2 screen_pos(
		widget_selected->getScreenPositionX(), widget_selected->getScreenPositionY());
	ImGui::DragInt2("Screen position", screen_pos);

	ImGui::Separator();
	if (ImGui::Button("Arrange"))
		widget_selected->arrange();

	ImGui::Separator();
	ImGui::Text("Font");

	int font_size = widget_selected->getFontSize();
	if (ImGui::DragInt("Size", &font_size))
		widget_selected->setFontSize(font_size);

	int font_permanent = widget_selected->getFontPermanent();
	if (ImGui::InputInt("Permanent", &font_permanent))
		widget_selected->setFontPermanent(font_permanent);

	Math::vec4 font_color = widget_selected->getFontColor();
	if (ImGui::ColorEdit4("Color", font_color))
		widget_selected->setFontColor(font_color);

	Math::ivec2 font_spacing(
		widget_selected->getFontHSpacing(), widget_selected->getFontVSpacing());
	if (ImGui::DragInt2("Spacing", font_spacing))
	{
		widget_selected->setFontHSpacing(font_spacing.x);
		widget_selected->setFontVSpacing(font_spacing.y);
	}

	Math::ivec2 font_offset(widget_selected->getFontHOffset(), widget_selected->getFontVOffset());
	if (ImGui::DragInt2("Offset", font_offset))
	{
		widget_selected->setFontHOffset(font_offset.x);
		widget_selected->setFontVOffset(font_offset.y);
	}

	int font_outline = widget_selected->getFontOutline();
	if (ImGui::DragInt("Outline", &font_outline))
		widget_selected->setFontOutline(font_outline);

	ImGui::Separator();

	switch (widget_selected->getType())
	{
		// TODO other types
	case Widget::WIDGET_VBOX:
	case Widget::WIDGET_HBOX:
	case Widget::WIDGET_GRID_BOX: {
		WidgetVBoxPtr vbox = checked_ptr_cast<WidgetVBox>(widget_selected);
		Math::vec4 vbox_color = vbox->getColor();
		if (ImGui::ColorEdit4("Vbox color", vbox_color))
			vbox->setColor(vbox_color);

		Math::vec4 vbox_bg_color = vbox->getBackgroundColor();
		if (ImGui::ColorEdit4("Background color", vbox_bg_color))
			vbox->setBackgroundColor(vbox_bg_color);
		break;
	}

	case Widget::WIDGET_LABEL: {
		WidgetLabelPtr label = checked_ptr_cast<WidgetLabel>(widget_selected);
		ImGui::Text("Label text: %s", label->getText());
		ImGui::Text("Label text align: %d", label->getTextAlign());
		break;
	}
	case Widget::WIDGET_SPRITE:
	case Widget::WIDGET_SPRITE_VIDEO:
	case Widget::WIDGET_SPRITE_SHADER:
	case Widget::WIDGET_SPRITE_VIEWPORT:
	case Widget::WIDGET_SPRITE_NODE: {
		WidgetSpritePtr sprite = checked_ptr_cast<WidgetSprite>(widget_selected);
		Math::vec4 sprite_color = sprite->getColor();
		if (ImGui::ColorEdit4("Sprite Color", sprite_color))
			sprite->setColor(sprite_color);
		ImGui::Text("Sprite texture: %s", sprite->getTexture());
		break;
	}
	default:	// nothing to do
		break;
	}
}
}	 // namespace

void WidgetsWindow::init()
{}

void WidgetsWindow::render(bool *p_open)
{
	if (ImGui::Begin("Gui Widgets", p_open))
	{
		if (ImGui::BeginTable("WidgetsParameters", 2,
				ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV
					| ImGuiTableFlags_Resizable))
		{
			ImGui::TableSetupScrollFreeze(0, 1);	// Make top row always visible
			ImGui::TableSetupColumn("Widgets", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Parameters", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();

			// widgets tree view
			ImGui::TableSetColumnIndex(0);
			/*
			ImGui::BeginChild("Filters", ImVec2(0, 60), true);
			static char str0[128] = "";
			ImGui::InputText("Filter", str0, IM_ARRAYSIZE(str0));
			ImGui::SameLine();
			if (ImGui::Button("X"))
				str0[0] = '\0';
			ImGui::Button("Collapse All");
			ImGui::SameLine();
			ImGui::Button("Expand All");
			ImGui::EndChild();
			*/
			ImGui::BeginChild("ChildWidgets", ImVec2(0, 0), false,
				ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysVerticalScrollbar);
			// if (str0[0] == '\0')
			draw_widgets_list();
			// else
			//	draw_filter_widgets_list(str0);
			ImGui::EndChild();

			// parameters
			ImGui::TableSetColumnIndex(1);
			ImGui::BeginChild("ChildWidgetParameters", ImVec2(0, 0), true);
			draw_widget_parameters();
			ImGui::EndChild();

			ImGui::EndTable();
		}
	}

	ImGui::End();
}

void WidgetsWindow::shutdown()
{}
