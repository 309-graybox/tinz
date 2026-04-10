// Copyright (C), UNIGINE. All rights reserved.

#include "CameraSettingsWindow.h"

#include "../imgui/imgui/imgui.h"

#include <UnigineGame.h>
#include <UniginePlayers.h>

using namespace Unigine;

void CameraSettingsWindow::init()
{}

void CameraSettingsWindow::render(bool *p_open)
{
	if (ImGui::Begin("Camera Settings", p_open))
	{
		PlayerPtr player = Game::getPlayer();
		if (player)
		{
			float value = player->getFov();
			if (ImGui::InputFloat("FOV", &value))
				player->setFov(value);

			value = player->getZNear();
			if (ImGui::InputFloat("Z Near", &value))
				player->setZNear(value);

			value = player->getZFar();
			if (ImGui::InputFloat("Z Far", &value))
				player->setZFar(value);
		}
	}
	ImGui::End();
}

void CameraSettingsWindow::shutdown()
{}
