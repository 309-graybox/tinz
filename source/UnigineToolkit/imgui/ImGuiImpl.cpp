// Copyright (C), UNIGINE. All rights reserved.

#include "ImGuiImpl.h"

#include <UnigineControls.h>
#include <UnigineEngine.h>
#include <UnigineFileSystem.h>
#include <UnigineGui.h>
#include <UnigineMaterials.h>
#include <UnigineMeshDynamic.h>
#include <UnigineRender.h>
#include <UnigineTextures.h>
#include <UnigineWindowManager.h>

using namespace Unigine;
using namespace Math;

// rendering
static TexturePtr font_texture;
static MeshDynamicPtr imgui_mesh;
static MaterialPtr imgui_material;
static ImDrawData *frame_draw_data;

// sprite to show UI
static WidgetSpritePtr imgui_w;
static TexturePtr imgui_tex;
static vec4 bgcolor = vec4(65 / 255.0f, 66 / 255.0f, 69 / 255.0f, 1.0f);

Unigine::EventConnections ImGuiImpl::event_connections;

float ImGuiImpl::last_scale = 1.0f;

ImGuiKey ImGui_UnigineKeyToImGuiKey(Input::KEY key)
{
	switch (key)
	{
		// clang-format off
		case Input::KEY_ESC: return ImGuiKey_Escape;
		case Input::KEY_F1: return ImGuiKey_F1;
		case Input::KEY_F2: return ImGuiKey_F2;
		case Input::KEY_F3: return ImGuiKey_F3;
		case Input::KEY_F4: return ImGuiKey_F4;
		case Input::KEY_F5: return ImGuiKey_F5;
		case Input::KEY_F6: return ImGuiKey_F6;
		case Input::KEY_F7: return ImGuiKey_F7;
		case Input::KEY_F8: return ImGuiKey_F8;
		case Input::KEY_F9: return ImGuiKey_F9;
		case Input::KEY_F10: return ImGuiKey_F10;
		case Input::KEY_F11: return ImGuiKey_F11;
		case Input::KEY_F12: return ImGuiKey_F12;
		case Input::KEY_PRINTSCREEN: return ImGuiKey_PrintScreen;
		case Input::KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
		case Input::KEY_PAUSE: return ImGuiKey_Pause;
		case Input::KEY_BACK_QUOTE: return ImGuiKey_GraveAccent;
		case Input::KEY_DIGIT_1: return ImGuiKey_1;
		case Input::KEY_DIGIT_2: return ImGuiKey_2;
		case Input::KEY_DIGIT_3: return ImGuiKey_3;
		case Input::KEY_DIGIT_4: return ImGuiKey_4;
		case Input::KEY_DIGIT_5: return ImGuiKey_5;
		case Input::KEY_DIGIT_6: return ImGuiKey_6;
		case Input::KEY_DIGIT_7: return ImGuiKey_7;
		case Input::KEY_DIGIT_8: return ImGuiKey_8;
		case Input::KEY_DIGIT_9: return ImGuiKey_9;
		case Input::KEY_DIGIT_0: return ImGuiKey_0;
		case Input::KEY_MINUS: return ImGuiKey_Minus;
		case Input::KEY_EQUALS: return ImGuiKey_Equal;
		case Input::KEY_BACKSPACE: return ImGuiKey_Backspace;
		case Input::KEY_TAB: return ImGuiKey_Tab;
		case Input::KEY_Q: return ImGuiKey_Q;
		case Input::KEY_W: return ImGuiKey_W;
		case Input::KEY_E: return ImGuiKey_E;
		case Input::KEY_R: return ImGuiKey_R;
		case Input::KEY_T: return ImGuiKey_T;
		case Input::KEY_Y: return ImGuiKey_Y;
		case Input::KEY_U: return ImGuiKey_U;
		case Input::KEY_I: return ImGuiKey_I;
		case Input::KEY_O: return ImGuiKey_O;
		case Input::KEY_P: return ImGuiKey_P;
		case Input::KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
		case Input::KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
		case Input::KEY_ENTER: return ImGuiKey_Enter;
		case Input::KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
		case Input::KEY_A: return ImGuiKey_A;
		case Input::KEY_S: return ImGuiKey_S;
		case Input::KEY_D: return ImGuiKey_D;
		case Input::KEY_F: return ImGuiKey_F;
		case Input::KEY_G: return ImGuiKey_G;
		case Input::KEY_H: return ImGuiKey_H;
		case Input::KEY_J: return ImGuiKey_J;
		case Input::KEY_K: return ImGuiKey_K;
		case Input::KEY_L: return ImGuiKey_L;
		case Input::KEY_SEMICOLON: return ImGuiKey_Semicolon;
		case Input::KEY_QUOTE: return ImGuiKey_Apostrophe;
		case Input::KEY_BACK_SLASH: return ImGuiKey_Backslash;
		case Input::KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
		//case Input::KEY_LESS: return ;
		case Input::KEY_Z: return ImGuiKey_Z;
		case Input::KEY_X: return ImGuiKey_X;
		case Input::KEY_C: return ImGuiKey_C;
		case Input::KEY_V: return ImGuiKey_V;
		case Input::KEY_B: return ImGuiKey_B;
		case Input::KEY_N: return ImGuiKey_N;
		case Input::KEY_M: return ImGuiKey_M;
		case Input::KEY_COMMA: return ImGuiKey_Comma;
		case Input::KEY_DOT: return ImGuiKey_Period;
		case Input::KEY_SLASH: return ImGuiKey_Slash;
		case Input::KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
		case Input::KEY_LEFT_CTRL: return ImGuiKey_LeftCtrl;
		case Input::KEY_LEFT_CMD: return ImGuiKey_LeftSuper;
		case Input::KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
		case Input::KEY_SPACE: return ImGuiKey_Space;
		case Input::KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
		case Input::KEY_RIGHT_CMD: return ImGuiKey_RightSuper;
		case Input::KEY_MENU: return ImGuiKey_Menu;
		case Input::KEY_RIGHT_CTRL: return ImGuiKey_RightCtrl;
		case Input::KEY_INSERT: return ImGuiKey_Insert;
		case Input::KEY_DELETE: return ImGuiKey_Delete;
		case Input::KEY_HOME: return ImGuiKey_Home;
		case Input::KEY_END: return ImGuiKey_End;
		case Input::KEY_PGUP: return ImGuiKey_PageUp;
		case Input::KEY_PGDOWN: return ImGuiKey_PageDown;
		case Input::KEY_UP: return ImGuiKey_UpArrow;
		case Input::KEY_LEFT: return ImGuiKey_LeftArrow;
		case Input::KEY_DOWN: return ImGuiKey_DownArrow;
		case Input::KEY_RIGHT: return ImGuiKey_RightArrow;
		case Input::KEY_NUM_LOCK: return ImGuiKey_NumLock;
		case Input::KEY_NUMPAD_DIVIDE: return ImGuiKey_KeypadDivide;
		case Input::KEY_NUMPAD_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case Input::KEY_NUMPAD_MINUS: return ImGuiKey_KeypadSubtract;
		case Input::KEY_NUMPAD_DIGIT_7: return ImGuiKey_Keypad7;
		case Input::KEY_NUMPAD_DIGIT_8: return ImGuiKey_Keypad8;
		case Input::KEY_NUMPAD_DIGIT_9: return ImGuiKey_Keypad9;
		case Input::KEY_NUMPAD_PLUS: return ImGuiKey_KeypadAdd;
		case Input::KEY_NUMPAD_DIGIT_4: return ImGuiKey_Keypad4;
		case Input::KEY_NUMPAD_DIGIT_5: return ImGuiKey_Keypad5;
		case Input::KEY_NUMPAD_DIGIT_6: return ImGuiKey_Keypad6;
		case Input::KEY_NUMPAD_DIGIT_1: return ImGuiKey_Keypad1;
		case Input::KEY_NUMPAD_DIGIT_2: return ImGuiKey_Keypad2;
		case Input::KEY_NUMPAD_DIGIT_3: return ImGuiKey_Keypad3;
		case Input::KEY_NUMPAD_ENTER: return ImGuiKey_KeypadEnter;
		case Input::KEY_NUMPAD_DIGIT_0: return ImGuiKey_Keypad0;
		case Input::KEY_NUMPAD_DOT: return ImGuiKey_KeypadDecimal;
	default:
		return ImGuiKey_None;
		// clang-format on
	}
}

static void update_key_modifiers(ImGuiIO &io)
{
	io.AddKeyEvent(ImGuiMod_Ctrl, Input::isKeyPressed(Input::KEY_ANY_CTRL));
	io.AddKeyEvent(ImGuiMod_Shift, Input::isKeyPressed(Input::KEY_ANY_SHIFT));
	io.AddKeyEvent(ImGuiMod_Alt, Input::isKeyPressed(Input::KEY_ANY_ALT));
	io.AddKeyEvent(ImGuiMod_Super, Input::isKeyPressed(Input::KEY_ANY_CMD));
}

static int on_key_pressed(Input::KEY key)
{
	auto &io = ImGui::GetIO();
	update_key_modifiers(io);
	io.AddKeyEvent(ImGui_UnigineKeyToImGuiKey(key), true);
	return 0;
}

static int on_key_released(Input::KEY key)
{
	auto &io = ImGui::GetIO();
	update_key_modifiers(io);
	io.AddKeyEvent(ImGui_UnigineKeyToImGuiKey(key), false);
	return 0;
}

#if 0
static int on_mouse_move(int x, int y)
{
	// does not work
	// is (x, y) relative to last pos?

	ivec2 pos;
	EngineWindowPtr main_window = WindowManager::getMainWindow();
	if (main_window)
		pos = main_window->getClientPosition();
	else
		pos = imgui_w->getParentGui()->getPosition();

	pos = ivec2(x,y) - pos;

	auto &io = ImGui::GetIO();
	io.AddMousePosEvent(static_cast<float>(pos.x), static_cast<float>(pos.y));
	return 0;
}
#endif

static int on_button_pressed(Input::MOUSE_BUTTON button)
{
	auto &io = ImGui::GetIO();
	update_key_modifiers(io);

	switch (button)
	{
	case Input::MOUSE_BUTTON_LEFT:
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
		break;
	case Input::MOUSE_BUTTON_RIGHT:
		io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
		break;
	case Input::MOUSE_BUTTON_MIDDLE:
		io.AddMouseButtonEvent(ImGuiMouseButton_Middle, true);
		break;
	default:	// nothing to do
		break;
	}

	return 0;
}

static int on_button_released(Input::MOUSE_BUTTON button)
{
	auto &io = ImGui::GetIO();
	update_key_modifiers(io);

	switch (button)
	{
	case Input::MOUSE_BUTTON_LEFT:
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
		break;
	case Input::MOUSE_BUTTON_RIGHT:
		io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
		break;
	case Input::MOUSE_BUTTON_MIDDLE:
		io.AddMouseButtonEvent(ImGuiMouseButton_Middle, false);
		break;
	default:	// nothing to do
		break;
	}

	return 0;
}

static int on_mouse_wheel(int v)
{
	auto &io = ImGui::GetIO();
	io.AddMouseWheelEvent(0, static_cast<float>(v));
	return 0;
}

static int on_mouse_wheel_h(int h)
{
	auto &io = ImGui::GetIO();
	io.AddMouseWheelEvent(static_cast<float>(h), 0);
	return 0;
}

static int on_unicode_key_pressed(unsigned int key)
{
	auto &io = ImGui::GetIO();
	io.AddInputCharacter(key);
	return 0;
}

static void set_clipboard_text(ImGuiContext *, const char *text)
{
	Input::setClipboard(text);
}

static char const *get_clipboard_text(ImGuiContext *)
{
	return Input::getClipboard();
}

static void create_font_texture()
{
	auto &io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF(
		FileSystem::getAbsolutePath("UnigineToolkit/imgui/Roboto-Light.ttf"), 13.225f, nullptr,
		io.Fonts->GetGlyphRangesCyrillic());

	unsigned char *pixels = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	font_texture = Texture::create();
	font_texture->create2D(width, height, Texture::FORMAT_RGBA8, Texture::SAMPLER_FILTER_LINEAR);

	auto blob = Blob::create();
	blob->setData(pixels, width * height * 32);
	font_texture->setBlob(blob);
	blob->setData(nullptr, 0);

	io.Fonts->TexID = (ImTextureID)(intptr_t)font_texture.get();
}

static void create_imgui_mesh()
{
	imgui_mesh = MeshDynamic::create(MeshDynamic::USAGE_DYNAMIC_ALL);

	MeshDynamic::Attribute attributes[3]{};
	attributes[0].offset = 0;
	attributes[0].size = 2;
	attributes[0].type = MeshDynamic::TYPE_FLOAT;
	attributes[1].offset = 8;
	attributes[1].size = 2;
	attributes[1].type = MeshDynamic::TYPE_FLOAT;
	attributes[2].offset = 16;
	attributes[2].size = 4;
	attributes[2].type = MeshDynamic::TYPE_UCHAR;
	imgui_mesh->setVertexFormat(attributes, 3);

	assert(imgui_mesh->getVertexSize() == sizeof(ImDrawVert)
		   && "Vertex size of MeshDynamic is not equal to size of ImDrawVert");
}

static void create_imgui_material()
{
	imgui_material = Materials::findManualMaterial("imgui")->inherit();
	if (!imgui_material)
		Log::error("Cound't find imgui material\n");
}

static void before_render_callback()
{
	auto &io = ImGui::GetIO();
	if (io.WantCaptureMouse)
	{
		Gui::getCurrent()->setMouseButtons(0);
	}
}

static void render_to_texture()
{
	if (!imgui_material)
		return;

	// resize texture with UI on it
	if (imgui_w->getRenderWidth() != imgui_tex->getWidth()
		|| imgui_w->getRenderHeight() != imgui_tex->getHeight())
	{
		imgui_tex->create2D(imgui_w->getRenderWidth(), imgui_w->getRenderHeight(),
			Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
		imgui_tex->clearBuffer();
		imgui_w->setRender(imgui_tex);
	}

	// clear with editor's color theme
	imgui_tex->clearBuffer(bgcolor);

	if (frame_draw_data == nullptr)
		return;

	auto draw_data = frame_draw_data;
	frame_draw_data = nullptr;

	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	auto render_target = Render::getTemporaryRenderTarget();
	render_target->bindColorTexture(0, imgui_tex);

	// Render state
	RenderState::saveState();
	RenderState::clearStates();
	RenderState::setBlendFunc(RenderState::BLEND_SRC_ALPHA, RenderState::BLEND_ONE_MINUS_SRC_ALPHA,
		RenderState::BLEND_OP_ADD);
	RenderState::setPolygonCull(RenderState::CULL_NONE);
	RenderState::setDepthFunc(RenderState::DEPTH_NONE);
	RenderState::setViewport(static_cast<int>(draw_data->DisplayPos.x),
		static_cast<int>(draw_data->DisplayPos.y), static_cast<int>(draw_data->DisplaySize.x),
		static_cast<int>(draw_data->DisplaySize.y));

	// Orthographic projection matrix
	float left = draw_data->DisplayPos.x;
	float right = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float top = draw_data->DisplayPos.y;
	float bottom = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

	Math::mat4 proj;
	proj.m00 = 2.0f / (right - left);
	proj.m03 = (right + left) / (left - right);
	proj.m11 = 2.0f / (top - bottom);
	proj.m13 = (top + bottom) / (bottom - top);
	proj.m22 = 0.5f;
	proj.m23 = 0.5f;
	proj.m33 = 1.0f;

	Renderer::setProjection(proj);
	auto shader = imgui_material->getShaderForce("imgui");
	auto pass = imgui_material->getRenderPass("imgui");
	Renderer::setShaderParameters(pass, shader, imgui_material, false);

	imgui_mesh->bind();

	// Write vertex and index data into dynamic mesh
	imgui_mesh->clearVertex();
	imgui_mesh->clearIndices();
	imgui_mesh->allocateVertex(draw_data->TotalVtxCount);
	imgui_mesh->allocateIndices(draw_data->TotalIdxCount);
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList *cmd_list = draw_data->CmdLists[i];

		imgui_mesh->addVertexArray(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size);
		imgui_mesh->addIndicesArray(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size);
	}
	imgui_mesh->flushVertex();
	imgui_mesh->flushIndices();

	render_target->enable();
	{
		int global_idx_offset = 0;
		int global_vtx_offset = 0;
		ImVec2 clip_off = draw_data->DisplayPos;
		// Draw command lists
		for (int i = 0; i < draw_data->CmdListsCount; ++i)
		{
			const ImDrawList *cmd_list = draw_data->CmdLists[i];
			for (int j = 0; j < cmd_list->CmdBuffer.Size; ++j)
			{
				const ImDrawCmd *cmd = &cmd_list->CmdBuffer[j];

				if (cmd->UserCallback != nullptr)
				{
					if (cmd->UserCallback != ImDrawCallback_ResetRenderState)
						cmd->UserCallback(cmd_list, cmd);
				}
				else
				{
					float width = (cmd->ClipRect.z - cmd->ClipRect.x) / draw_data->DisplaySize.x;
					float height = (cmd->ClipRect.w - cmd->ClipRect.y) / draw_data->DisplaySize.y;
					float x = (cmd->ClipRect.x - clip_off.x) / draw_data->DisplaySize.x;
					float y =
						1.0f - height - (cmd->ClipRect.y - clip_off.y) / draw_data->DisplaySize.y;

					RenderState::setScissorTest(x, y, width, height);
					RenderState::flushStates();

					auto texture = TexturePtr((Texture *)(cmd->GetTexID()));
					RenderState::setTexture(RenderState::BIND_FRAGMENT, 0, texture);

					imgui_mesh->bind();
					imgui_mesh->renderInstancedSurface(MeshDynamic::MODE_TRIANGLES,
						cmd->VtxOffset + global_vtx_offset, cmd->IdxOffset + global_idx_offset,
						cmd->IdxOffset + global_idx_offset + cmd->ElemCount, 1);
				}
			}
			global_vtx_offset += cmd_list->VtxBuffer.Size;
			global_idx_offset += cmd_list->IdxBuffer.Size;
		}

		RenderState::setScissorTest(0.0f, 0.0f, 1.0f, 1.0f);
	}
	render_target->disable();
	imgui_mesh->unbind();

	RenderState::restoreState();

	render_target->unbindColorTexture(0);
	Render::releaseTemporaryRenderTarget(render_target);
}

void ImGuiImpl::init(const Unigine::WidgetPtr &root_widget)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	Input::getEventKeyDown().connect(event_connections, on_key_pressed);
	Input::getEventKeyUp().connect(event_connections, on_key_released);

	// Input::getEventMouseMotion().connect(event_connections, on_mouse_move);
	Input::getEventMouseDown().connect(event_connections, on_button_pressed);
	Input::getEventMouseUp().connect(event_connections, on_button_released);
	Input::getEventMouseWheel().connect(event_connections, on_mouse_wheel);
	Input::getEventMouseWheelHorizontal().connect(event_connections, on_mouse_wheel_h);

	Input::getEventTextPress().connect(event_connections, on_unicode_key_pressed);
	Engine::get()->getEventBeginRender().connect(event_connections, before_render_callback);

	// sprite to show UI
	GuiPtr main_gui = root_widget->getParentGui();
	imgui_w = WidgetSprite::create("white.texture");
	imgui_w->setOrder(128);	   // fix for UIDesigner order changing
	main_gui->addChild(imgui_w, Gui::ALIGN_OVERLAP);
	imgui_tex = Texture::create();
	imgui_tex->create2D(main_gui->getWidth(), main_gui->getHeight(), Texture::FORMAT_RGBA8,
		Texture::FORMAT_USAGE_RENDER);
	imgui_tex->clearBuffer();
	imgui_w->setRender(imgui_tex);

	ImGuiIO &io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

	io.BackendPlatformName = "imgui_impl_unigine";
	io.BackendRendererName = "imgui_impl_unigine";

	ImGuiPlatformIO &pio = ImGui::GetPlatformIO();

	pio.Platform_SetClipboardTextFn = set_clipboard_text;
	pio.Platform_GetClipboardTextFn = get_clipboard_text;
	pio.Platform_ClipboardUserData = nullptr;

	create_font_texture();
	create_imgui_mesh();
	create_imgui_material();

	// set editor's color theme
	ImGui::StyleColorsDark();
	ImVec4 *colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.25f, 0.26f, 0.27f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.33f, 0.34f, 0.35f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.39f, 0.40f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.38f, 0.39f, 0.40f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.44f, 0.45f, 0.47f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.37f, 0.38f, 0.39f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.41f, 0.42f, 0.43f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.28f, 0.51f, 0.62f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.16f, 0.31f, 0.42f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	colors[ImGuiCol_Tab] = ImVec4(0.21f, 0.21f, 0.22f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.24f, 0.41f, 0.52f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.24f, 0.41f, 0.52f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.67f, 0.05f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
}

void ImGuiImpl::newFrame()
{
	GuiPtr main_gui = imgui_w->getParentGui();
	imgui_w->setPosition(0, 0);
	imgui_w->setWidth(main_gui->getWidth());
	imgui_w->setHeight(main_gui->getHeight());

	ivec2 context_pos;
	ivec2 context_size;
	EngineWindowPtr main_window = WindowManager::getMainWindow();
	if (main_window)
	{
		context_size = main_window->getClientRenderSize();
		context_pos = main_window->getClientPosition();
	}
	else
	{
		context_pos = main_gui->getPosition();
		ivec2 unit_size = main_gui->getSize();
		context_size.x = main_gui->toRenderSize(unit_size.x);
		context_size.y = main_gui->toRenderSize(unit_size.y);
	}

	// input/output
	auto &io = ImGui::GetIO();

	Unigine::ControlsApp::setEnabled(!io.WantCaptureKeyboard);

	io.DisplaySize = ImVec2(Math::toFloat(context_size.x), Math::toFloat(context_size.y));
	io.DeltaTime = Engine::get()->getIFps();

	if (io.WantSetMousePos)
		Input::setMousePosition(Math::ivec2(Math::ftoi(io.MousePos.x), Math::ftoi(io.MousePos.y)));

	const Math::ivec2 mouse_coord = Input::getMousePosition() - context_pos;
	io.AddMousePosEvent(static_cast<float>(mouse_coord.x), static_cast<float>(mouse_coord.y));

	float scale = main_gui->getDpiScale();
	if (Math::compare(last_scale, scale) == 0)
	{
		last_scale = scale;
		ImGui::GetStyle().ScaleAllSizes(scale);
		io.FontGlobalScale = scale;
	}

	ImGui::NewFrame();
}

void ImGuiImpl::render()
{
	ImGui::Render();
	frame_draw_data = ImGui::GetDrawData();
	render_to_texture();
}

void ImGuiImpl::shutdown()
{
	imgui_w.deleteForce();

	imgui_material.deleteForce();

	event_connections.disconnectAll();

	ImGui::DestroyContext();
}

bool ImGuiImpl::isWantCaptureMouse()
{
	return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiImpl::isWantCaptureKeyboard()
{
	return ImGui::GetIO().WantCaptureKeyboard;
}

WidgetSpritePtr &ImGuiImpl::getWidget()
{
	return imgui_w;
}

void ImGuiImpl::setBackgroundColor(const vec4 &color)
{
	bgcolor = color;
}

void ImGuiImpl::bringToFront()
{
	imgui_w->getParent()->addChild(imgui_w, Gui::ALIGN_OVERLAP);
}
