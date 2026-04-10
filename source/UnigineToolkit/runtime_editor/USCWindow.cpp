// Copyright (C), UNIGINE. All rights reserved.

#include "USCWindow.h"

#include "../imgui/imgui-color-text-edit/TextEditor.h"
#include "../imgui/imgui/imgui.h"

#include <UnigineEngine.h>
#include <UnigineInput.h>
#include <UnigineInterpreter.h>

using namespace Unigine;

namespace {
TextEditor *editor = nullptr;
float font_scale = 1.0f;
String compile_errors;
String output;

StringStack<> datef(const char *format)
{
	StringStack<> ret;

	if (format == NULL)
		return ret;

	time_t t = (time_t)time(NULL);

#ifdef UNIGINE_PS5
	struct tm _tm;
	localtime_s(&t, &_tm);
#elif _WIN32
	struct tm _tm;
	localtime_s(&_tm, &t);
#else
	struct tm _tm;
	localtime_r(&t, &_tm);
#endif

	static const char *days[7] = {
		"Sun",
		"Mon",
		"Tue",
		"Wed",
		"Thu",
		"Fri",
		"Sat",
	};
	static const char *months[12] = {
		"Jan",
		"Feb",
		"Mar",
		"Apr",
		"May",
		"Jun",
		"Jul",
		"Aug",
		"Sep",
		"Oct",
		"Nov",
		"Dec",
	};

	StringStack<> buf;
	StringStack<> token;
	const char *s = format;
	while (*s)
	{
		if (*s == '%')
		{
			buf.clear();
			token.clear();
			token.append(*s++);
			while (*s && strchr("0123456789.-#", *s))
				token.append(*s++);
			token.append('d');
			if (*s == '\0')
				Log::error("date(): bad format \"%s\"\n", format);
			else if (*s == 'a')
				buf = days[_tm.tm_wday];
			else if (*s == 'b')
				buf = months[_tm.tm_mon];
			else if (*s == 'd')
				buf.printf("%s %s %d, %02d:%02d", days[_tm.tm_wday], months[_tm.tm_mon],
					_tm.tm_mday, _tm.tm_hour, _tm.tm_min);
			else if (*s == 's')
				buf.printf(token, _tm.tm_sec);
			else if (*s == 'm')
				buf.printf(token, _tm.tm_min);
			else if (*s == 'h')
				buf.printf(token, _tm.tm_hour);
			else if (*s == 'D')
				buf.printf(token, _tm.tm_mday);
			else if (*s == 'M')
				buf.printf(token, _tm.tm_mon + 1);
			else if (*s == 'Y')
				buf.printf(token, _tm.tm_year + 1900);
			else if (*s == 'W')
				buf.printf(token, _tm.tm_wday + 1);
			else if (*s == '%')
				buf = "%";
			else
				Log::error("date(): unknown format \"%c\" in \"%s\" string\n", *s, format);
			ret.append(buf);
			s++;
		}
		else
			ret.append(*s++);
	}

	return ret;
}

void error_handler(const char *msg)
{
	compile_errors += msg;
}

// executing UnigineScript code
void run_expression()
{
	void *system_interp = Engine::get()->getSystemInterpreter();
	std::string text = editor->GetText();

	compile_errors.clear();
	EventConnection connection;
	Log::getEventError().connectUnsafe(error_handler);
	Expression e = Expression(system_interp, text.c_str());
	output = compile_errors;

	if (e.isCompiled())
	{
		output =
			String::format("Compilation successfull (%s)! Executed.", datef("%2h:%02m:%02s").get());
		e.run();
	}
	else
	{
		output += String::format("\nCompilation failed (%s).", datef("%2h:%02m:%02s").get());
	}
}
}	 // namespace

void USCWindow::init()
{
	editor = new TextEditor();
	editor->SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());

	editor->SetText("{\n\tlog.message(\"UnigineScript!\\n\");\n}");
}

void USCWindow::render(bool *p_open)
{
	if (ImGui::Begin("UnigineScript's Code Executor", p_open,
			ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar))
	{
		// hotkeys
		if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::getMouseWheel() != 0)
		{
			font_scale += 0.1f * Input::getMouseWheel();
			font_scale = Math::max(1.0f, font_scale);
		}
		if (Input::isKeyDown(Input::KEY_F5))
			run_expression();

		// menu
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Interpreter"))
			{
				if (ImGui::MenuItem("Run", "F5"))
					run_expression();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, editor->CanUndo()))
					editor->Undo();
				if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, editor->CanRedo()))
					editor->Redo();

				ImGui::Separator();

				if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor->HasSelection()))
					editor->Copy();
				if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, editor->HasSelection()))
					editor->Cut();
				if (ImGui::MenuItem("Delete", "Del", nullptr, editor->HasSelection()))
					editor->Delete();
				if (ImGui::MenuItem(
						"Paste", "Ctrl-V", nullptr, ImGui::GetClipboardText() != nullptr))
					editor->Paste();

				ImGui::Separator();

				if (ImGui::MenuItem("Select all", nullptr, nullptr))
					editor->SetSelection(TextEditor::Coordinates(),
						TextEditor::Coordinates(editor->GetTotalLines(), 0));

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::BeginMenu("Style"))
				{
					if (ImGui::MenuItem("Dark"))
						editor->SetPalette(TextEditor::GetDarkPalette());
					if (ImGui::MenuItem("Light"))
						editor->SetPalette(TextEditor::GetLightPalette());
					if (ImGui::MenuItem("Retro Blue"))
						editor->SetPalette(TextEditor::GetRetroBluePalette());
					ImGui::EndMenu();
				}

				bool whitespaces = editor->IsShowingWhitespaces();
				if (ImGui::MenuItem("Show White Space and TAB", nullptr, &whitespaces))
					editor->SetShowWhitespaces(whitespaces);

				if (ImGui::BeginMenu("Font Scale"))
				{
					if (ImGui::MenuItem("1.0"))
						font_scale = 1.0f;
					if (ImGui::MenuItem("1.25"))
						font_scale = 1.25f;
					if (ImGui::MenuItem("1.5"))
						font_scale = 1.5f;
					if (ImGui::MenuItem("1.75"))
						font_scale = 1.75f;
					if (ImGui::MenuItem("2.0"))
						font_scale = 2.0f;
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		// status bar
		auto cpos = editor->GetCursorPosition();
		ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1,
			editor->GetTotalLines(), editor->IsOverwrite() ? "Ovr" : "Ins",
			editor->CanUndo() ? "*" : " ", editor->GetLanguageDefinition().mName.c_str());

		ImGui::InputTextMultiline("##output", &output.get(0), output.size(),
			ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 4), ImGuiInputTextFlags_ReadOnly);

		// context
		editor->Render("TextEditor", font_scale);
	}
	ImGui::End();
}

void USCWindow::shutdown()
{
	delete editor;
	editor = nullptr;
}
