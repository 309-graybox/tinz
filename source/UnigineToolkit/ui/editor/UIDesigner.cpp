#include "UIDesigner.h"

#include "../../imgui/ImGuiImpl.h"
#include "../../imgui/imgui/imgui_internal.h"
#include "../elements/ProgressBar.h"
#include "../elements/SpriteShader.h"

#include <UnigineConsole.h>
#include <UnigineEditor.h>
#include <UnigineTextures.h>
#include <UnigineWindowManager.h>

#ifdef EDITOR_PLUGIN
	#include <editor/UnigineActions.h>
	#include <editor/UnigineAssetDialogs.h>
	#include <editor/UnigineConstants.h>
	#include <editor/UnigineEngineGuiWindow.h>
	#include <editor/UnigineSelection.h>
	#include <editor/UnigineSelector.h>
	#include <editor/UnigineWindowManager.h>
#endif

#include "UndoCommands.h"

using namespace Unigine;
using namespace Math;

namespace {
#define show_label_macro(name, getter, value_type, format)                                         \
	{                                                                                              \
		bool _mixed = false;                                                                       \
		value_type value = elements[0]->getter();                                                  \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != value)                                                    \
			{                                                                                      \
				_mixed = true;                                                                     \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (_mixed)                                                                                \
			ImGui::Text("%s: -", name);                                                            \
		else                                                                                       \
			ImGui::Text(format, name, value);                                                      \
	}

#define show_label_string_macro(name, getter)                                                      \
	{                                                                                              \
		StringStack<> value = elements[0]->getter();                                               \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != value)                                                    \
			{                                                                                      \
				value = "-";                                                                       \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		ImGui::Text("%s: %s", name, value.get());                                                  \
	}

#define show_field_macro(name, getter, setter, value_type, func, out_type)                         \
	{                                                                                              \
		bool _mixed = false;                                                                       \
		value_type value = elements[0]->getter();                                                  \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != value)                                                    \
			{                                                                                      \
				_mixed = true;                                                                     \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (_mixed)                                                                                \
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);                                  \
		if (func(name, value, c, r))                                                               \
		{                                                                                          \
			for (int i = 0; i < elements.size(); i++)                                              \
				elements[i]->setter(out_type);                                                     \
		}                                                                                          \
		if (_mixed)                                                                                \
			ImGui::PopItemFlag();                                                                  \
	}

#define show_field_iter_macro(name, getter, setter, iter, value_type, func, out_type)              \
	{                                                                                              \
		bool _mixed = false;                                                                       \
		value_type value = elements[0]->getter(iter);                                              \
		for (int _i = 1; _i < elements.size(); _i++)                                               \
		{                                                                                          \
			if (elements[_i]->getter(iter) != value)                                               \
			{                                                                                      \
				_mixed = true;                                                                     \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (_mixed)                                                                                \
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, true);                                  \
		if (func(name, value, c, r))                                                               \
		{                                                                                          \
			for (int _i = 0; _i < elements.size(); _i++)                                           \
				elements[_i]->setter(iter, out_type);                                              \
		}                                                                                          \
		if (_mixed)                                                                                \
			ImGui::PopItemFlag();                                                                  \
	}

#define show_field_string_macro(name, getter, setter)                                              \
	{                                                                                              \
		Unigine::StringStack<> value = elements[0]->getter();                                      \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != value)                                                    \
			{                                                                                      \
				value = "-";                                                                       \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (parameter_string(name, value, c, r))                                                   \
		{                                                                                          \
			for (int i = 0; i < elements.size(); i++)                                              \
				elements[i]->setter(out_str);                                                      \
		}                                                                                          \
	}

#define show_field_string_image_macro(name, getter, setter)                                        \
	{                                                                                              \
		Unigine::StringStack<> value = elements[0]->getter();                                      \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != value)                                                    \
			{                                                                                      \
				value = "-";                                                                       \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (parameter_string_image(name, value, MakeCallback([elements, &c, &r](const String &s) { \
				for (int i = 0; i < elements.size(); i++)                                          \
					elements[i]->setter(s);                                                        \
				c = r = true;                                                                      \
			}),                                                                                    \
				c, r))                                                                             \
		{                                                                                          \
			for (int i = 0; i < elements.size(); i++)                                              \
				elements[i]->setter(out_str);                                                      \
		}                                                                                          \
	}

#define show_field_material_macro(name, getter, setter)                                            \
	{                                                                                              \
		bool _mixed = false;                                                                       \
		Unigine::MaterialPtr _value = elements[0]->getter();                                       \
		for (int _i = 1; _i < elements.size(); _i++)                                               \
		{                                                                                          \
			if (elements[_i]->getter() != _value)                                                  \
			{                                                                                      \
				_mixed = true;                                                                     \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
		if (_mixed)                                                                                \
		{                                                                                          \
			ImGuiContext &g = *GImGui;                                                             \
			g.NextItemData.ItemFlags |= ImGuiItemFlags_MixedValue;                                 \
		}                                                                                          \
		if (parameter_material(name, _value,                                                       \
				MakeCallback([elements, &c, &r](const MaterialPtr &m) {                            \
					for (int _i = 0; _i < elements.size(); _i++)                                   \
						elements[_i]->setter(m);                                                   \
					c = r = true;                                                                  \
				}),                                                                                \
				c, r))                                                                             \
		{                                                                                          \
			for (int _i = 0; _i < elements.size(); _i++)                                           \
				elements[_i]->setter(out_mat);                                                     \
		}                                                                                          \
	}

#define show_field_string_iter_macro(name, getter, setter, iter)                                   \
	{                                                                                              \
		Unigine::StringStack<> value = elements[0]->getter(iter);                                  \
		for (int _i = 1; _i < elements.size(); _i++)                                               \
		{                                                                                          \
			if (elements[_i]->getter(iter) != value)                                               \
			{                                                                                      \
				value = "-";                                                                       \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (parameter_string(name, value, c, r))                                                   \
		{                                                                                          \
			for (int _i = 0; _i < elements.size(); _i++)                                           \
				elements[_i]->setter(iter, out_str);                                               \
		}                                                                                          \
	}

#define show_field_string_image_iter_macro(name, getter, setter, iter)                             \
	{                                                                                              \
		Unigine::StringStack<> _value = elements[0]->getter(iter);                                 \
		for (int _i = 1; _i < elements.size(); _i++)                                               \
		{                                                                                          \
			if (elements[_i]->getter(iter) != _value)                                              \
			{                                                                                      \
				_value = "-";                                                                      \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (parameter_string_image(name, _value,                                                   \
				MakeCallback([elements, iter, &c, &r](const String &s) {                           \
					for (int _i = 0; _i < elements.size(); _i++)                                   \
						elements[_i]->setter(iter, s);                                             \
					c = r = true;                                                                  \
				}),                                                                                \
				c, r))                                                                             \
		{                                                                                          \
			for (int _i = 0; _i < elements.size(); _i++)                                           \
				elements[_i]->setter(iter, out_str);                                               \
		}                                                                                          \
	}

#define show_field_bool_macro(name, getter, setter)                                                \
	{                                                                                              \
		bool _mixed = false;                                                                       \
		bool value = elements[0]->getter();                                                        \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != value)                                                    \
			{                                                                                      \
				_mixed = true;                                                                     \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (_mixed)                                                                                \
		{                                                                                          \
			ImGuiContext &g = *GImGui;                                                             \
			g.NextItemData.ItemFlags |= ImGuiItemFlags_MixedValue;                                 \
		}                                                                                          \
		if (parameter_bool(name, value, c, r))                                                     \
		{                                                                                          \
			for (int i = 0; i < elements.size(); i++)                                              \
				elements[i]->setter(out_bool);                                                     \
		}                                                                                          \
	}

#define show_field_combo_macro(name, getter, setter, type, items)                                  \
	{                                                                                              \
		bool _mixed = false;                                                                       \
		int value = (int)elements[0]->getter();                                                    \
		for (int i = 1; i < elements.size(); i++)                                                  \
		{                                                                                          \
			if (elements[i]->getter() != (type)value)                                              \
			{                                                                                      \
				_mixed = true;                                                                     \
				break;                                                                             \
			}                                                                                      \
		}                                                                                          \
                                                                                                   \
		if (_mixed)                                                                                \
		{                                                                                          \
			ImGui::TextUnformatted(name);                                                          \
			ImGui::SameLine();                                                                     \
			if (ImGui::Button("(mixed)"))                                                          \
			{                                                                                      \
				for (int i = 0; i < elements.size(); i++)                                          \
					elements[i]->setter((type)0);                                                  \
				c = r = true;                                                                      \
			}                                                                                      \
		}                                                                                          \
		else if (ImGui::Combo(name, &value, items, IM_ARRAYSIZE(items)))                           \
		{                                                                                          \
			for (int i = 0; i < elements.size(); i++)                                              \
				elements[i]->setter((type)value);                                                  \
			c = r = true;                                                                          \
		}                                                                                          \
	}

#define show_label_bool(name, get_method)	show_label_macro(name, get_method, bool, "%s: %d")
#define show_label_string(name, get_method) show_label_string_macro(name, get_method)
#define show_label_int(name, get_method)	show_label_macro(name, get_method, int, "%s: %d")
#define show_label_float(name, get_method)	show_label_macro(name, get_method, float, "%s: %f")

#define show_field_bool(name, get_method, set_method)                                              \
	show_field_bool_macro(name, get_method, set_method)
#define show_field_string(name, get_method, set_method)                                            \
	show_field_string_macro(name, get_method, set_method)
#define show_field_string_image(name, get_method, set_method)                                      \
	show_field_string_image_macro(name, get_method, set_method)
#define show_field_material(name, get_method, set_method)                                          \
	show_field_material_macro(name, get_method, set_method)
#define show_field_int(name, get_method, set_method)                                               \
	show_field_macro(name, get_method, set_method, int, parameter_int, out_int)
#define show_field_float(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, float, parameter_float, out_float)
#define show_field_float_angle(name, get_method, set_method)                                       \
	show_field_macro(name, get_method, set_method, float, parameter_float_angle, out_float)
#define show_field_double(name, get_method, set_method)                                            \
	show_field_macro(name, get_method, set_method, double, parameter_double, out_double)
#define show_field_vec2(name, get_method, set_method)                                              \
	show_field_macro(name, get_method, set_method, vec2, parameter_vec2, out_vec2)
#define show_field_vec3(name, get_method, set_method)                                              \
	show_field_macro(name, get_method, set_method, vec3, parameter_vec3, out_vec3)
#define show_field_vec4(name, get_method, set_method)                                              \
	show_field_macro(name, get_method, set_method, vec4, parameter_vec4, out_vec4)
#define show_field_dvec2(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, dvec2, parameter_dvec2, out_dvec2)
#define show_field_dvec3(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, dvec3, parameter_dvec3, out_dvec3)
#define show_field_dvec4(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, dvec4, parameter_dvec4, out_dvec4)
#define show_field_ivec2(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, ivec2, parameter_ivec2, out_ivec2)
#define show_field_ivec3(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, ivec3, parameter_ivec3, out_ivec3)
#define show_field_ivec4(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, ivec4, parameter_ivec4, out_ivec4)
#define show_field_color(name, get_method, set_method)                                             \
	show_field_macro(name, get_method, set_method, vec4, parameter_color, out_color)
#define show_field_combo(name, get_method, set_method, type, items)                                \
	show_field_combo_macro(name, get_method, set_method, type, items)

#define show_field_string_i(name, get_method, set_method, iter)                                    \
	show_field_string_iter_macro(name, get_method, set_method, iter)
#define show_field_string_image_i(name, get_method, set_method, iter)                              \
	show_field_string_image_iter_macro(name, get_method, set_method, iter)
#define show_field_int_i(name, get_method, set_method, iter)                                       \
	show_field_iter_macro(name, get_method, set_method, iter, int, parameter_int, out_int)
#define show_field_ivec2_i(name, get_method, set_method, iter)                                     \
	show_field_iter_macro(name, get_method, set_method, iter, ivec2, parameter_ivec2, out_ivec2)
#define show_field_ivec3_i(name, get_method, set_method, iter)                                     \
	show_field_iter_macro(name, get_method, set_method, iter, ivec3, parameter_ivec3, out_ivec3)
#define show_field_ivec4_i(name, get_method, set_method, iter)                                     \
	show_field_iter_macro(name, get_method, set_method, iter, ivec4, parameter_ivec4, out_ivec4)
#define show_field_float_i(name, get_method, set_method, iter)                                     \
	show_field_iter_macro(name, get_method, set_method, iter, float, parameter_float, out_float)
#define show_field_vec2_i(name, get_method, set_method, iter)                                      \
	show_field_iter_macro(name, get_method, set_method, iter, vec2, parameter_vec2, out_vec2)
#define show_field_vec3_i(name, get_method, set_method, iter)                                      \
	show_field_iter_macro(name, get_method, set_method, iter, vec3, parameter_vec3, out_vec3)
#define show_field_vec4_i(name, get_method, set_method, iter)                                      \
	show_field_iter_macro(name, get_method, set_method, iter, vec4, parameter_vec4, out_vec4)
#define show_field_color_i(name, get_method, set_method, iter)                                     \
	show_field_iter_macro(name, get_method, set_method, iter, vec4, parameter_color, out_color)

bool out_bool;
char out_str[0xFFF] = "";	 // tmp variable (parameter string)
int out_int;
float out_float;
double out_double;
vec2 out_vec2;
vec3 out_vec3;
vec4 out_vec4;
dvec2 out_dvec2;
dvec3 out_dvec3;
dvec4 out_dvec4;
ivec2 out_ivec2;
ivec3 out_ivec3;
ivec4 out_ivec4;
vec4 out_color;
MaterialPtr out_mat;

bool IsItemActiveLastFrame()
{
	ImGuiContext &g = *GImGui;
	if (g.ActiveIdPreviousFrame)
		return g.ActiveIdPreviousFrame == ImGui::GetItemID();
	return false;
}
// replace with IsItemDeactivated or IsItemDeactivatedAfterEdit
bool IsItemJustReleased()
{
	return IsItemActiveLastFrame() && !ImGui::IsItemActive();
}

bool parameter_bool(const char *label, bool value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_bool = value;
	if (ImGui::Checkbox(label, &out_bool))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_string(const char *label, const char *value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_str[0] = '\0';
	memcpy(out_str, value, strlen(value) + 1 /*'\0'*/);
	if (ImGui::InputText(label, out_str, IM_ARRAYSIZE(out_str)))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_string_image(const char *label, const char *value,
	Unigine::CallbackBase1<Unigine::String> *func, bool &changed, bool &released)
{
	bool has_changed = false;
	out_str[0] = '\0';
	memcpy(out_str, value, strlen(value) + 1 /*'\0'*/);
	if (ImGui::InputText(label, out_str, IM_ARRAYSIZE(out_str)))
		has_changed = true;
	ImGui::SameLine();
	if (ImGui::Button(String::format("...##%s", label)))
	{
		if (Engine::get()->isEditorLoaded())
		{
#ifdef EDITOR_PLUGIN
			UnigineEditor::AssetDialogs::browseAsset(
				MakeCallback([func](const UnigineEditor::AssetDialogs::SelectedAsset &asset) {
					if (func)
					{
						String path =
							String::format("guid://%s", asset.runtime_guid.makeString().get());
						func->run(path);
					}
				}),
				"Select Image", ".png.jpg.tga.tif.texture");
#endif
		}
	}
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_material(const char *label, const Unigine::MaterialPtr &value,
	Unigine::CallbackBase1<Unigine::MaterialPtr> *func, bool &changed, bool &released)
{
	bool has_changed = false;
	StringStack<> mat_guid =
		value ? (value->getParent() ? value->getParent()->getFileGUID().makeString()
									: value->getFileGUID().makeString())
			  : nullptr;
	out_str[0] = '\0';
	memcpy(out_str, mat_guid, strlen(mat_guid) + 1 /*'\0'*/);
	if (ImGui::InputText(label, out_str, IM_ARRAYSIZE(out_str)))
	{
		out_mat = Materials::findMaterialByFileGUID(UGUID(out_str));
		has_changed = true;
	}
	ImGui::SameLine();
	if (ImGui::Button(String::format("...##%s", label)))
	{
		if (Engine::get()->isEditorLoaded())
		{
#ifdef EDITOR_PLUGIN
			UnigineEditor::AssetDialogs::browseAsset(
				MakeCallback([func](const UnigineEditor::AssetDialogs::SelectedAsset &asset) {
					if (func)
					{
						MaterialPtr mat = Materials::findMaterialByFileGUID(asset.asset_guid);
						func->run(mat);
					}
				}),
				"Select Material", ".mat.basemat");
#endif
		}
	}

	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_int(const char *label, int value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_int = value;
	if (ImGui::InputInt(label, &out_int))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_float(const char *label, float value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_float = value;
	if (ImGui::InputFloat(label, &out_float, 0, 0, "%.2f"))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_float_angle(const char *label, float value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_float = value;
	if (ImGui::SliderFloat(label, &out_float, 0.0f, 360.0f, "%.2f"))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_double(const char *label, double value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_double = value;
	if (ImGui::InputDouble(label, &out_double, 0, 0, "%.5f"))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_vec2(const char *label, const vec2 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_vec2 = value;
	if (ImGui::InputFloat2(label, out_vec2, "%.2f"))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_vec3(const char *label, const vec3 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_vec3 = value;
	if (ImGui::InputFloat3(label, out_vec3, "%.2f"))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_vec4(const char *label, const vec4 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_vec4 = value;
	if (ImGui::InputFloat4(label, out_vec4, "%.2f"))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_ivec2(const char *label, const ivec2 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_ivec2 = value;
	if (ImGui::InputInt2(label, out_ivec2))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_ivec3(const char *label, const ivec3 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_ivec3 = value;
	if (ImGui::InputInt3(label, out_ivec3))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_ivec4(const char *label, const ivec4 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_ivec4 = value;
	if (ImGui::InputInt4(label, out_ivec4))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

bool parameter_color(const char *label, const vec4 &value, bool &changed, bool &released)
{
	bool has_changed = false;
	out_color = value;
	if (ImGui::ColorEdit4(label, out_color))
		has_changed = true;
	changed |= has_changed;
	released |= IsItemJustReleased();
	return has_changed;
}

Input::KEY enable_hotkey = Input::KEY::KEY_F3;
}	 // namespace

UIDesigner *UIDesigner::get()
{
	static UIDesigner instance;
	return &instance;
}

void UIDesigner::setHotkeyEnabled(Input::KEY hotkey)
{
	enable_hotkey = hotkey;
}

Input::KEY UIDesigner::getHotkeyEnabled()
{
	return enable_hotkey;
}

void UIDesigner::init(const Unigine::GuiPtr &gui, bool use_custom_render)
{
	if (initialized)
		return;

	if (use_custom_render)
	{
		background = WidgetSprite::create("white.texture");
		background->setColor(vec4(0.6f * 0.255f, 0.6f * 0.259f, 0.6f * 0.27f, 1.0f));
		gui->addChild(background, Gui::ALIGN_BACKGROUND | Gui::ALIGN_EXPAND);

		gui_sprite = WidgetSprite::create();
		gui_sprite->setBlendFunc(Gui::BLEND_ONE, Gui::BLEND_ZERO);
		gui->addChild(gui_sprite, Gui::ALIGN_OVERLAP);

		if (UI::Canvas::get() || UI::Canvas::getAllCanvases().size())
		{
			canvas = UI::Canvas::get() ? UI::Canvas::get()->getPtr()
									   : UI::Canvas::getAllCanvases()[0]->getPtr();
			TexturePtr render_tex = canvas->getRenderTexture();
			if (render_tex)
				render_tex->clear();
		}
	}

	canvas_widget = WidgetCanvas::create();
	canvas_widget->addLine();	 // manipulator: edges
	canvas_widget->addLine();	 // manipulator: vertices (4 circle corners)
	canvas_widget->addLine();
	canvas_widget->addLine();
	canvas_widget->addLine();
	canvas_widget->addPolygon();	// selection: background
	canvas_widget->addLine();		// selection: edges
	canvas_widget->addLine();		// selected element: pivot
	canvas_widget->addLine();		// selected element: parent's anchor rectangle
	canvas_widget->addLine();		// selected element: anchor rectangle
	canvas_widget->addLine();		// selected element: anchor LT
	canvas_widget->addLine();		// selected element: anchor LB
	canvas_widget->addLine();		// selected element: anchor RB
	canvas_widget->addLine();		// selected element: anchor RT
	canvas_widget->addLine();		// align line: horizontal
	canvas_widget->addLine();		// align line: vertical
	gui->addChild(canvas_widget, Gui::ALIGN_OVERLAP);

	if (WindowManager::getMainWindow())
		dpi_scale = WindowManager::getMainWindow()->getDpiScale();
	else
		dpi_scale = 1.0f;

	file_dialog = WidgetDialogFile::create(gui, "File Dialog");

	undo_manager = new UndoManager();
#ifndef EDITOR_PLUGIN
	create_trash_root();
#endif

	find_all_creator_elements();

#ifdef EDITOR_PLUGIN
	Engine::get()->addEditorLogic(&editor_logic);
#endif

#ifndef EDITOR_PLUGIN
	saved_mouse_handle = Input::getMouseHandle();
#endif

	initialized = true;
}

void UIDesigner::init(bool use_custom_render)
{
	init(Gui::getCurrent(), use_custom_render);
}

void UIDesigner::update()
{
	if (!initialized)
		return;

	bool editor_loaded = Engine::get()->isEditorLoaded();

	if (!editor_loaded)
	{
		if (Input::isKeyDown(enable_hotkey) && !Console::isActive())
			setShow(!isShow());

		if (!show)
			return;
	}

	// get current window size
	if (WindowManager::getMainWindow())
		window_size = WindowManager::getMainWindow()->getClientSize();
	else
	{
		ivec2 unit_size = canvas_widget->getGui()->getSize();
		window_size = ivec2(canvas_widget->getGui()->toRenderSize(unit_size.x),
			canvas_widget->getGui()->toRenderSize(unit_size.y));
	}

	if (canvas)
	{
		canvas->needToRearrange();	  // fix positions after buttons like "Expand All"
		canvas->updateManual(Engine::get()->getIFps());
	}

	if (gui_sprite && canvas)
	{
		gui_sprite->setHidden(false);

		// render UI
		if (use_render_as_background)
			gui_sprite->setRender(
				canvas->render(render_width, render_height, Editor::getPlayer(), bg_render_fade));
		else
			gui_sprite->setRender(canvas->render(render_width, render_height, gui_sprite_bg_color));
		int gui_width = gui_sprite->getGui()->getWidth();
		int gui_height = gui_sprite->getGui()->getHeight();

		gui_sprite->setHeight(ftoi(render_height * gui_sprite_pos_scale.z));
		gui_sprite->setWidth(ftoi(render_width * gui_sprite_pos_scale.z));
		gui_sprite->arrange();
		gui_sprite->setPosition(ftoi(gui_width / 2 - gui_sprite->getWidth() * 0.5f
									 + gui_sprite_pos_scale.x * gui_sprite->getHeight()),
			ftoi(gui_height / 2 - gui_sprite->getHeight() * 0.5f
				 + gui_sprite_pos_scale.y * gui_sprite->getHeight()));

		if (!ImGui::GetIO().WantCaptureMouse)
		{
			GuiPtr gui = canvas_widget->getGui();
			int wheel = gui->getMouseWheel();
			if (wheel != 0)
			{
				gui_sprite_pos_scale.z *= 1.0f + wheel * 0.1f;
				/*
				gui_sprite_pos_scale.z += wheel * 0.25f;
				if (gui_sprite_pos_scale.z < 0.25f)
					gui_sprite_pos_scale.z = 0.25f;
				*/
			}
			bool middle = gui->getMouseButtons() == 2;
			if (middle)
			{
				gui_sprite_pos_scale.x += itof(gui->getMouseDX()) / gui_sprite->getHeight();
				gui_sprite_pos_scale.y += itof(gui->getMouseDY()) / gui_sprite->getHeight();
			}
		}
	}
	else if (gui_sprite && gui_sprite->getRender() && !canvas)
		gui_sprite->setHidden(true);

	// bring to front, sort UI
	canvas_widget->raise(canvas_widget);
	ImGuiImpl::bringToFront();

	// selection
	check_node_selection();
	mouse_update();
	keyboard_update();
	check_selection_existence();
	update_selection_parameters();
	update_manipulator();
	update_canvas();

	// render UI
	if (!editor_loaded)
		ImGuiImpl::newFrame();

	render_main_menu();
	render_toolset();
	render_hierarchy();
	render_parameters();
	render_hotkeys();
	need_to_reset_layout = false;

	mouse_context_menu_update();

	if (!editor_loaded)
		ImGuiImpl::render();
}

void UIDesigner::shutdown()
{
	if (!initialized)
		return;

#ifdef EDITOR_PLUGIN
	Engine::get()->removeEditorLogic(&editor_logic);
#endif

	gui_sprite.deleteLater();
	delete selected_element_changes;
	selected_element_changes = nullptr;
	delete undo_manager;
	undo_manager = nullptr;
#ifndef EDITOR_PLUGIN
	trash_root.deleteForce();
#endif
	selected_elements.clear();
	show = false;
	canvas_widget.deleteLater();

	canvas.clear();
	all_creator_elements.clear();
	clipboard.clear();
	editor_selected_nodes.clear();

#ifndef EDITOR_PLUGIN
	Input::setMouseHandle(saved_mouse_handle);
#endif

	initialized = false;
}

void UIDesigner::setShow(bool in_show)
{
	auto prev = show;
	show = in_show;
	if (prev != show)
		update_show_state();
}

#ifndef EDITOR_PLUGIN
void UIDesigner::create_trash_root()
{
	if (trash_root)
		return;

	NodeDummyPtr trash_root_node = NodeDummy::create();
	trash_root_node->setEnabled(false);
	trash_root_node->setShowInEditorEnabled(false);
	trash_root_node->setSaveToWorldEnabled(false);
	auto component = ComponentSystem::get()->addComponent<UI::Element>(trash_root_node);
	component->applyNodeHierarchyChanges(false);
	trash_root = component->getPtr();
}
#endif

void UIDesigner::update_show_state()
{
	auto imgui_w = ImGuiImpl::getWidget();
	if (show)
	{
#ifndef EDITOR_PLUGIN
		saved_mouse_handle = Input::getMouseHandle();
		Input::setMouseHandle(Input::MOUSE_HANDLE_USER);
		Input::setMouseCursorHide(false);
#endif

		if (gui_sprite)
		{
			background->setHidden(false);
			gui_sprite->setHidden(false);
		}
		ImGuiImpl::getWidget()->setHidden(false);
		ImGuiImpl::bringToFront();
	}
	else
	{
#ifndef EDITOR_PLUGIN
		Input::setMouseHandle(saved_mouse_handle);
#endif

		if (gui_sprite)
		{
			background->setHidden(true);
			gui_sprite->setHidden(true);
		}
		ImGuiImpl::getWidget()->setHidden(true);
		clear_canvas();
	}
}

void UIDesigner::clear_canvas()
{
	canvas_widget->clearLinePoints(0);	  // manipulator: edges
	canvas_widget->clearLinePoints(1);	  // manipulator: vertices (4 circle corners)
	canvas_widget->clearLinePoints(2);
	canvas_widget->clearLinePoints(3);
	canvas_widget->clearLinePoints(4);
	canvas_widget->clearPolygonPoints(5);	 // selection: background
	canvas_widget->clearLinePoints(6);		 // selection: edges
	canvas_widget->clearLinePoints(7);		 // selected element: pivot
	canvas_widget->clearLinePoints(8);		 // selected element: parent's anchor rectangle
	canvas_widget->clearLinePoints(9);		 // selected element: anchor rectangle
	canvas_widget->clearLinePoints(10);		 // selected element: anchor LT
	canvas_widget->clearLinePoints(11);		 // selected element: anchor LB
	canvas_widget->clearLinePoints(12);		 // selected element: anchor RB
	canvas_widget->clearLinePoints(13);		 // selected element: anchor RT
	canvas_widget->clearLinePoints(14);		 // align line: horizontal
	canvas_widget->clearLinePoints(15);		 // align line: vertical
}

void UIDesigner::render_main_menu()
{
	bool open_create_popup = false;
	bool open_resize_popup = false;

	ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration;
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(itof(canvas_widget->getGui()->getWidth()), 0.0f));
	ImGui::Begin("UIDesigner", nullptr, flags);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Canvas"))
		{
			if (ImGui::MenuItem("Create..."))
				open_create_popup = true;
			if (ImGui::MenuItem("Resize...", nullptr, nullptr, canvas ? true : false))
				open_resize_popup = true;
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z"))
				undo_manager->undo();
			if (ImGui::MenuItem("Redo", "Ctrl+Y"))
				undo_manager->redo();
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "Ctrl+X", false, selected_elements.size() > 0))
				cut_selected();
			if (ImGui::MenuItem("Copy", "Ctrl+C", false, selected_elements.size() > 0))
				copy_to_clipboard();
			if (ImGui::MenuItem("Paste", "Ctrl+V", false, clipboard.size() > 0))
				paste_from_clipboard();
			if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selected_elements.size() > 0))
				duplicate_selected();
			if (ImGui::MenuItem("Delete", "Del", false, selected_elements.size() > 0))
				destroy_selected();
			ImGui::Separator();
			if (ImGui::MenuItem("Select All", "Ctrl+A"))
				select_all();
			if (ImGui::MenuItem("Deselect All", "Alt+A"))
				deselect_all();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Anchor", nullptr, &view_anchor);
			ImGui::MenuItem("Pivot", nullptr, &view_pivot);
			ImGui::MenuItem("Manipulator", nullptr, &view_manipulator);
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Toolset", nullptr, &window_toolset);
			ImGui::MenuItem("Hierarchy", nullptr, &window_hierarchy);
			ImGui::MenuItem("Parameters", nullptr, &window_parameters);
			ImGui::MenuItem("Hotkeys", nullptr, &window_hotkeys);
			ImGui::Separator();
			if (ImGui::MenuItem("Reset Windows Layout"))
			{
				need_to_reset_layout = true;
				window_toolset = true;
				window_hierarchy = true;
				window_parameters = true;
				window_hotkeys = true;
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	///////////////////////////////////////////////////////////////////////////////////

	// background color
	if (gui_sprite)
	{
		ImGui::Checkbox("##use_render_as_bg##", &use_render_as_background);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Use Render as Background");

		ImGui::SameLine();
		if (use_render_as_background)
		{
			ImGui::ColorEdit4(
				"##background_fade_color##", bg_render_fade, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Background Fade Color");
		}
		else
		{
			ImGui::ColorEdit3(
				"##background_color##", gui_sprite_bg_color, ImGuiColorEditFlags_NoInputs);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Background Color");
		}
	}

	// show list of canvases
	{
		int selected_canvas_index = -1;
		String canvases_list;

		auto all_canvases = UI::Canvas::getAllCanvases();
		VectorStack<UI::Canvas *> canvases;
		for (int i = 0; i < all_canvases.size(); i++)
			if (all_canvases[i]->getNode()->isShowInEditorEnabled())
				canvases.append(all_canvases[i]);

		size_t num_chars = 0;
		for (int i = 0; i < canvases.size(); i++)
			num_chars += strlen(canvases[i]->getName()) + 1;
		canvases_list.resize(static_cast<int>(num_chars));
		int index = 0;
		for (int i = 0; i < canvases.size(); i++)
		{
			const char *item_name = canvases[i]->getName();
#ifdef _LINUX
			strcpy(&(canvases_list[index]), item_name);
#else
			strcpy_s(&(canvases_list[index]), canvases_list.size(), item_name);
#endif
			index += static_cast<int>(strlen(item_name));
			canvases_list[index] = '\0';
			index++;

			if (canvas && canvas.get() == canvases[i])
				selected_canvas_index = i;
		}
		if (selected_canvas_index == -1)
		{
			canvas.clear();
			selected_canvas_index = 0;
		}
		if (gui_sprite)
			ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f);
		if (ImGui::Combo(
				"##canvases##", &selected_canvas_index, canvases_list, IM_ARRAYSIZE(canvases_list))
			|| (!canvas && canvases.size()))
		{
			canvas = canvases[selected_canvas_index]->getPtr();
			clear_selection();

			// hide all widgets except selected canvas
			for (int i = 0; i < canvases.size(); i++)
				canvases[i]->getGui()->setHidden(i != selected_canvas_index);
		}
	}

	// canvas resolution info
	if (canvas)
	{
		ImGui::SameLine();
		if (canvas->canvas_mode.get() == 0)
		{
			ImGui::TextUnformatted(String::format(
				"Canvas: %dx%d", canvas->getScreenWidth(), canvas->getScreenHeight()));
		}
		else
			ImGui::TextUnformatted(String::format("Canvas: %dx%d",
				ftoi(canvas->getCanvasReferenceWidth()), ftoi(canvas->getCanvasReferenceHeight())));
	}

	// render settings
	if (gui_sprite && canvas)
	{
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

		ImGui::SameLine();
		ImGui::TextUnformatted("  Render:");
		ImGui::SameLine();
		const char *render_modes[] = {"Custom", "1024x768 (4:3 XGA)", "1280x720 (16:9 HD)",
			"1920x1080 (16:9 FullHD)", "1920x1200 (16:10)", "2560x1080 (21:9)",
			"2560x1440 (16:9 2K)", "3840x2160 (16:9 4K)", "7680x2160 (32:9)", "Window Size"};
		static int render_mode = 0;
		static int custom_render_size[]{1920, 1080};
		auto align_canvas_scale = [this]() {
			int prev_h = canvas->getRenderTexture()->getHeight();
			float prev_scale = gui_sprite_pos_scale.z;
			int new_h = render_height;
			gui_sprite_pos_scale.z = prev_scale * prev_h / new_h;
		};
		ImGui::SetNextItemWidth(160.0f);
		if (ImGui::Combo("##render_mode##", &render_mode, render_modes, IM_ARRAYSIZE(render_modes)))
		{
			switch (render_mode)
			{
			case 0:	   // Custom
				render_width = custom_render_size[0];
				render_height = custom_render_size[1];
				align_canvas_scale();
				break;
			case 1:	   // "1024x768 (4:3 XGA)"
				render_width = 1024;
				render_height = 768;
				align_canvas_scale();
				break;
			case 2:	   // "1280x720 (16:9 HD)"
				render_width = 1280;
				render_height = 720;
				align_canvas_scale();
				break;
			case 3:	   // "1920x1080 (16:9 FullHD)"
				render_width = 1920;
				render_height = 1080;
				align_canvas_scale();
				break;
			case 4:	   // "1920x1200 (16:10)"
				render_width = 1920;
				render_height = 1200;
				align_canvas_scale();
				break;
			case 5:	   // "2560x1080 (21:9)"
				render_width = 2560;
				render_height = 1080;
				align_canvas_scale();
				break;
			case 6:	   // "2560x1440 (16:9 2K)"
				render_width = 2560;
				render_height = 1440;
				align_canvas_scale();
				break;
			case 7:	   // "3840x2160 (16:9 4K)"
				render_width = 3840;
				render_height = 2160;
				align_canvas_scale();
				break;
			case 8:	   // "7680x2160 (32:9)"
				render_width = 7680;
				render_height = 2160;
				align_canvas_scale();
				break;
			case 9:	   // "Window Size"
				gui_sprite_pos_scale = vec3(0, 0, 1);
				break;
			}
		}
		if (render_mode == 0)	 // "Custom"
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(100.0f);
			if (ImGui::InputInt2("##resolution##", custom_render_size))
			{
				custom_render_size[0] = clamp(custom_render_size[0], 1, 8192);
				custom_render_size[1] = clamp(custom_render_size[1], 1, 8192);
				render_width = custom_render_size[0];
				render_height = custom_render_size[1];
				align_canvas_scale();
			}
		}
		if (render_mode == 9)	 // "Window Size"
		{
			render_width = canvas_widget->getGui()->getWidth();
			render_height = canvas_widget->getGui()->getHeight();
		}

		ImGui::SameLine();
		ImGui::Text("Zoom: %.0f%%", gui_sprite_pos_scale.z * 100.0f);

		ImGui::SameLine();
		if (ImGui::Button("Fit"))
		{
			if (render_mode != 9)
			{
				int window_header = 60;
				int visible_width = window_size.x;
				int visible_height = window_size.y - window_header;
				float scale_x = itof(visible_width) / render_width;
				float scale_y = itof(visible_height) / render_height;
				float canvas_scale = Math::min(scale_x, scale_y);
				float scaled_height = render_height * canvas_scale;
				gui_sprite_pos_scale.x = 0;
				gui_sprite_pos_scale.y =
					((visible_height - scaled_height) * 0.5f + window_header * 0.5f)
					/ window_size.y;
				gui_sprite_pos_scale.z = canvas_scale;
			}
			else
				gui_sprite_pos_scale = vec3(0, 0, 1);
		}
		ImGui::SameLine();
		if (ImGui::Button("1:1"))
			gui_sprite_pos_scale.z = 1.0f;
		ImGui::SameLine();
		if (ImGui::Button("2:1"))
			gui_sprite_pos_scale.z = 2.0f;
	}

	// snap to elements
	{
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

		ImGui::SameLine();
		ImGui::Checkbox("##use_snap_elements##", &use_snap_to_elements);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Use snap to other elements");
	}

	// snap to grid
	{
		ImGui::SameLine();
		ImGui::Checkbox("##use_snap_grid##", &use_snap_to_grid);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Use snap to grid");
		if (use_snap_to_grid)
		{
			ImGui::SameLine();
			ImGui::TextUnformatted("Grid Size");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(90.0f);
			ImGui::InputInt("##grid_size##", &grid_size);
		}
	}

	// sync selection
	if (gui_sprite)
	{
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

		ImGui::SameLine();
		ImGui::Checkbox("##use_sync##", &sync_selection);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Synchronize selection between UI and nodes in the Editor");
	}

	///////////////////////////////////////////////////////////////////////////////////

	ImGui::End();

	//////////////////////////////////////////////////////////////////////
	// popups

	// canvas/create...
	if (open_create_popup)
		ImGui::OpenPopup("Create Canvas");
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Create Canvas", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char canvas_name[128] = "canvas";
		ImGui::InputText("Name", canvas_name, IM_ARRAYSIZE(canvas_name));

		const char *items[] = {"Constant Pixel Size", "Reference Resolution"};
		static int item_current = 1;
		ImGui::Combo("Canvas Mode", &item_current, items, IM_ARRAYSIZE(items));

		static bool use_dpi_scale = true;
		static float pixel_scale = 1;
		static float reference_width = 1920;
		static float reference_height = 1080;
		if (item_current == 0)
		{
			ImGui::Checkbox("Use DPI Scale", &use_dpi_scale);
			ImGui::InputFloat("Pixel Scale", &pixel_scale);
		}
		else if (item_current == 1)
		{
			ImGui::InputFloat("Reference Width", &reference_width);
			ImGui::InputFloat("Reference Height", &reference_height);
		}

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			create_canvas_node();
			canvas->setName(canvas_name);
			canvas->canvas_mode = item_current;
			canvas->use_dpi_scale = use_dpi_scale ? 1 : 0;
			canvas->pixel_scale = pixel_scale;
			canvas->reference_width = reference_width;
			canvas->reference_height = reference_height;

			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	// canvas/resize...
	if (open_resize_popup)
		ImGui::OpenPopup("Resize Canvas");
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Resize Canvas", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static bool recalc_positions = true;
		ImGui::Checkbox("Recalculate all positions to save current visual", &recalc_positions);

		static int item_current = 1;

		static bool use_dpi_scale = true;
		static float pixel_scale = 1;
		static float reference_width = 1920;
		static float reference_height = 1080;

		if (open_resize_popup)
		{
			item_current = canvas->canvas_mode;
			use_dpi_scale = canvas->use_dpi_scale.get() != 0;
			pixel_scale = canvas->pixel_scale;
			reference_width = canvas->reference_width;
			reference_height = canvas->reference_height;
		}

		const char *items[] = {"Constant Pixel Size", "Reference Resolution"};
		ImGui::Combo("Canvas Mode", &item_current, items, IM_ARRAYSIZE(items));
		if (item_current == 0)
		{
			ImGui::Checkbox("Use DPI Scale", &use_dpi_scale);
			ImGui::InputFloat("Pixel Scale", &pixel_scale);
		}
		else if (item_current == 1)
		{
			ImGui::InputFloat("Reference Width", &reference_width);
			ImGui::InputFloat("Reference Height", &reference_height);
		}

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			// 1. find all screen positions
			Unigine::Vector<ivec4> all_screen_pos;
			std::function<void(UI::Element *)> func_get;
			func_get = [&](UI::Element *parent) {
				for (int i = 0; i < parent->getNumChildren(); i++)
				{
					UI::Element *c = parent->getChild(i);
					const vec2 &n_min = c->getNormalizedBoundMin();
					const vec2 &n_max = c->getNormalizedBoundMax();
					vec2 c_min = vec2(
						n_min.x * canvas->getCanvasWidth(), n_min.y * canvas->getCanvasHeight());
					vec2 c_max = vec2(
						n_max.x * canvas->getCanvasWidth(), n_max.y * canvas->getCanvasHeight());
					ivec2 s_min = canvas->convertCanvasToScreen(c_min);
					ivec2 s_max = canvas->convertCanvasToScreen(c_max);
					all_screen_pos.append(ivec4(s_min, s_max));

					func_get(c);
				}
			};
			if (recalc_positions)
				func_get(canvas.get());

			// 2. change canvas
			canvas->canvas_mode = item_current;
			canvas->use_dpi_scale = use_dpi_scale ? 1 : 0;
			canvas->pixel_scale = pixel_scale;
			canvas->reference_width = reference_width;
			canvas->reference_height = reference_height;
			canvas->updateManual(0);	// refresh values

			// 3. recalculate and apply positions
			int s_index = 0;
			std::function<void(UI::Element *)> func_set;
			func_set = [&](UI::Element *parent) {
				for (int i = 0; i < parent->getNumChildren(); i++)
				{
					UI::Element *c = parent->getChild(i);
					ivec4 s_p = all_screen_pos[s_index];
					s_index++;

					vec2 c0 = canvas->convertScreenToCanvas(ivec2(s_p.x, s_p.y));
					vec2 c1 = canvas->convertScreenToCanvas(ivec2(s_p.z, s_p.w));
					c->setWorldPosition(c0, c1);

					func_set(c);
				}
			};
			if (recalc_positions)
				func_set(canvas.get());

			ImGui::CloseCurrentPopup();
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	//////////////////////////////////////////////////////////////////////
}

void UIDesigner::render_toolset()
{
	if (!window_toolset)
		return;

	ImGui::SetNextWindowPos(
		ImVec2(0, 354.0f), need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(150, window_size.y - 152.0f - 354.0f),
		need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::Begin("Toolset", &window_toolset);

	ImVec2 button_size = ImVec2(130.0f, 0.0f);

	if (!canvas)
	{
		if (ImGui::Button("Create Canvas", button_size))
			create_canvas_node();
	}
	else
	{
		for (int i = 0; i < all_creator_elements.size(); i++)
		{
			bool selected = selected_creator == all_creator_elements[i];
			if (selected)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
				ImGui::PushStyleColor(
					ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
				ImGui::PushStyleColor(
					ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));
			}
			if (ImGui::Button(all_creator_elements[i].get() + 3, button_size))
			{
				if (!selected)
					selected_creator = all_creator_elements[i];	   // select
				else
					selected_creator.clear();	 // deselect
			}
			if (selected)
				ImGui::PopStyleColor(3);
		}
	}

	ImGui::End();
}

void UIDesigner::render_hierarchy()
{
	if (!window_hierarchy)
		return;

	ImGui::SetNextWindowPos(
		ImVec2(0, 54.0f), need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(
		ImVec2(150, 300), need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::Begin("Hierarchy", &window_hierarchy);
	ImGui::BeginChild("HierarchyTree", ImVec2(0, 0), false,
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysVerticalScrollbar);
	if (canvas)
		render_hierarchy_element(canvas->getPtr());
	ImGui::EndChild();
	ImGui::End();

	need_to_focus = false;
}

void UIDesigner::render_hierarchy_element(const UI::ElementPtr &element)
{
	if (!element)
		return;
#ifndef EDITOR_PLUGIN
	if (element == trash_root)	  // hidden element for deleted widgets
		return;
#endif

	if (selected_parents.size() && selected_parents.last() == element)
	{
		ImGui::SetNextItemOpen(true);
		selected_parents.removeLast();
	}

	StringStack<> name = element->getNode()->getName();
	if (name.empty())
		name = String::format("[%s]", element->getPropertyName());

	static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow
										   | ImGuiTreeNodeFlags_OpenOnDoubleClick
										   | ImGuiTreeNodeFlags_SpanAvailWidth;
	ImGuiTreeNodeFlags node_flags = base_flags;
	if (selected_elements.contains(element))
		node_flags |= ImGuiTreeNodeFlags_Selected;

	auto handle_selection = [this](const UI::ElementPtr &element) {
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			drag_select.pending_element = element;
			drag_select.mouse_down_pos = vec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
			drag_select.waiting_for_drag = true;
		}

		// start dragging...
		if (drag_select.waiting_for_drag && drag_select.pending_element == element)
		{
			float dist = length2(vec2(ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y)
								 - drag_select.mouse_down_pos);
			if (dist >= drag_threshold * drag_threshold)
				drag_select.waiting_for_drag = false;
		}

		// start selecting...
		if (drag_select.waiting_for_drag && drag_select.pending_element == element
			&& ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			select_element_by_user(drag_select.pending_element, false);
			drag_select.waiting_for_drag = false;
			drag_select.pending_element.clear();
		}
	};

	// helper func to check if the first element is a child of the parent (recursively)
	std::function<bool(UI::Element *, const UI::Element *)> is_child_of =
		[&](UI::Element *e, const UI::Element *parent) {
			if (e == nullptr || parent == nullptr)
				return false;
			if (e == parent)
				return true;
			if (is_child_of(e->getParent(), parent))
				return true;
			return false;
		};

	auto drag_drop_reparent = [this, is_child_of](const UI::ElementPtr &element) {
		if (drag_select.waiting_for_drag)
			return;

		if (drag_select.pending_element == element
			&& ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			static Vector<UI::ElementPtr> payload;

			// choose what to drag: current selection or new element (that not selected yet)
			bool dragging_selected_element = false;
			for (int i = 0; i < selected_elements.size(); i++)
				if (selected_elements[i] == drag_select.pending_element)
				{
					dragging_selected_element = true;
					break;
				}
			if (dragging_selected_element)
				payload = selected_roots;
			else
			{
				payload.clear();
				payload.append(drag_select.pending_element);
			}

			ImGui::SetDragDropPayload(
				"NODE_MOVE", payload.get(), payload.size() * sizeof(UI::ElementPtr));

			StringStack<> s;
			for (int i = 0; i < Math::min(payload.size(), 3); i++)
			{
				StringStack<> name = payload[i]->getNode()->getName();
				if (name.empty())
					name = String::format("[%s]", payload[i]->getPropertyName());
				s += name + "\n";
			}
			if (payload.size() > 3)
				s += "...";
			ImGui::TextUnformatted(s.get());
			ImGui::EndDragDropSource();
		}

		// reparenting
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("NODE_MOVE"))
			{
				const UI::ElementPtr *moved_element = (UI::ElementPtr *)payload->Data;
				int count = payload->DataSize / sizeof(UI::ElementPtr);

				VectorStack<UI::ElementPtr> moved_elements;
				for (int i = 0; i < count; ++i)
					moved_elements.append(moved_element[i]);

				// check if we try parent move to its children
				bool is_child_of_element = false;
				for (int i = 0; i < count; i++)
					if (is_child_of(element.get(), moved_elements[i].get()))
					{
						is_child_of_element = true;
						break;
					}

				// do
				if (!is_child_of_element)
					undo_manager->apply(new ReparentWidgetCommand(moved_elements, element));
			}
			ImGui::EndDragDropTarget();
		}
	};

	auto draw_drop_reorder = [this, is_child_of](
								 const UI::ElementPtr &ref_element, bool place_before) {
		if (!ImGui::IsDragDropActive() || !ref_element->getParent())
			return;

		ImVec2 start = ImGui::GetCursorScreenPos();
		ImVec2 size = ImVec2(ImGui::GetContentRegionAvail().x, 4.0f);

		ImGui::InvisibleButton(
			String::format("##dropzone_%d%d", ref_element.get(), place_before), size);

		// draw line when hovering
		if (ImGui::GetMousePos().y >= start.y && ImGui::GetMousePos().y <= start.y + size.y)
			ImGui::GetWindowDrawList()->AddLine(ImVec2(start.x, start.y - 1),
				ImVec2(start.x + size.x, start.y - 1), IM_COL32(128, 200, 255, 255), 1.0f);

		ImGui::SetCursorScreenPos(start);	 // to prevent moving new elements after

		// reordering
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("NODE_MOVE"))
			{
				const UI::ElementPtr *moved_element = (UI::ElementPtr *)payload->Data;
				int count = payload->DataSize / sizeof(UI::ElementPtr);

				VectorStack<UI::ElementPtr> moved_elements;
				for (int i = 0; i < count; ++i)
					moved_elements.append(moved_element[i]);

				// check if we try parent move to its children
				bool is_child_of_element = false;
				for (int i = 0; i < count; i++)
					if (is_child_of(ref_element.get(), moved_elements[i].get()))
					{
						is_child_of_element = true;
						break;
					}

				// do
				if (!is_child_of_element)
					undo_manager->apply(
						new ReorderWidgetCommand(moved_elements, ref_element, place_before));
			}
			ImGui::EndDragDropTarget();
		}
	};

	if (element->getNumChildren())
	{
		ImGui::PushStyleColor(ImGuiCol_Text,
			element->getNode()->isEnabled() ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		bool node_open = ImGui::TreeNodeEx((void *)element.get(), node_flags, "%s", name.get());
		if (need_to_focus && (node_flags & ImGuiTreeNodeFlags_Selected))
			ImGui::SetScrollHereY();
		ImGui::PopStyleColor();
		handle_selection(element);
		drag_drop_reparent(element);

		if (node_open)
		{
			draw_drop_reorder(element->getChild(0)->getPtr(), true);
			for (int i = 0; i < element->getNumChildren(); i++)
			{
				render_hierarchy_element(element->getChild(i)->getPtr());
			}
			ImGui::TreePop();
		}
		else
			draw_drop_reorder(element, false);
	}
	else
	{
		node_flags |= ImGuiTreeNodeFlags_Leaf
					  | ImGuiTreeNodeFlags_NoTreePushOnOpen;	// ImGuiTreeNodeFlags_Bullet
		ImGui::PushStyleColor(ImGuiCol_Text,
			element->getNode()->isEnabled() ? ImVec4(1, 1, 1, 1) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::TreeNodeEx((void *)element.get(), node_flags, "%s", name.get());
		if (need_to_focus && (node_flags & ImGuiTreeNodeFlags_Selected))
			ImGui::SetScrollHereY();
		ImGui::PopStyleColor();
		handle_selection(element);
		drag_drop_reparent(element);
		draw_drop_reorder(element, false);
	}
}

void UIDesigner::render_parameters()
{
	if (!window_parameters)
		return;
	ImGui::SetNextWindowPos(ImVec2(itof(window_size.x - 290), 54),
		need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(290, itof(window_size.y - 54)),
		need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::Begin("Parameters", &window_parameters);
	if (selected_elements.size() && !skip_parameter_window)
	{
		render_base_parameters(selected_elements, parameter_changed, parameter_released);
		render_derived_parameters(selected_elements, parameter_changed, parameter_released);

		// store to undo/redo system
		if (parameter_changed && parameter_released)
		{
			selected_element_changes->saveState();
			undo_manager->push(selected_element_changes);
			selected_element_changes = nullptr;
			refresh_selected_element_changes();
		}
	}
	skip_parameter_window = false;
	ImGui::End();
}

void UIDesigner::render_hotkeys()
{
	if (!window_hotkeys)
		return;

	ImGui::SetNextWindowPos(ImVec2(0, window_size.y - 152.0f),
		need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(
		ImVec2(150, 152), need_to_reset_layout ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
	ImGui::Begin("Hotkeys", &window_hotkeys);

	ImGui::TextUnformatted(
		"Ctrl + D: duplicate\n"
		"Ctrl + X/C/V: cut/copy/paste\n"
		"Ctrl + Z: undo\n"
		"Ctrl + Y: redo\n"
		"Delete: remove\n"
		"Arrows: move\n"
		"Ctrl: snap x10, set pivot only\n"
		"Shift + Move: one axis\n"
		"Shift + Scale: lock aspect\n");

	ImGui::End();
}

void UIDesigner::render_base_parameters(
	const Unigine::Vector<UI::ElementPtr> &elements, bool &c, bool &r)
{
	static bool recalc_position = true;
	static bool change_pivot_too = true;

	show_field_bool("##Enabled", getNode()->isEnabled, getNode()->setEnabled);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Enabled");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(165);
	show_field_string("Name", getNode()->getName, getNode()->setName);
	show_label_string("Type", getClassName);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::BeginChild("position", ImVec2(0, 265), true);

	vec4 anc = elements[0]->getAnchor();
	show_field_float(anc.x == anc.z ? "Pos X" : "Left Offset", getPositionX, setPositionX);
	show_field_float(anc.y == anc.w ? "Pos Y" : "Top Offset", getPositionY, setPositionY);
	show_field_float(anc.x == anc.z ? "Width" : "Right Offset", getWidth, setWidth);
	show_field_float(anc.y == anc.w ? "Height" : "Bottom Offset", getHeight, setHeight);

	show_field_vec4("Anchor", getAnchor, setAnchor);
	show_field_vec2("Pivot", getPivot, setPivot);
	ImGui::Separator();

	vec4 stored_pos;
	ImVec2 button_size = ImVec2(58.0f, 0.0f);

	ImGui::Checkbox("Recalculate Positions", &recalc_position);
	if (!Input::isKeyPressed(Input::KEY_ANY_CTRL))
	{
		ImGui::SameLine();
		ImGui::Checkbox("Change Pivot too", &change_pivot_too);
	}

#define change_anchor(func, pivot)                                                                 \
	for (int i = 0; i < elements.size(); i++)                                                      \
	{                                                                                              \
		if (recalc_position)                                                                       \
			stored_pos = elements[i]->getWorldPosition();                                          \
		if (Input::isKeyPressed(Input::KEY_ANY_CTRL))                                              \
			elements[i]->setPivot(pivot);                                                          \
		else                                                                                       \
			elements[i]->func(change_pivot_too);                                                   \
		if (recalc_position)                                                                       \
			elements[i]->setWorldPosition(stored_pos);                                             \
	}                                                                                              \
	c = r = true;

	button_size = ImVec2(80.0f, 0.0f);

	if (ImGui::Button("Left Top", button_size))
	{
		change_anchor(setAnchorLeftTop, vec2(0, 0));
	}
	ImGui::SameLine();
	if (ImGui::Button("Center Top", button_size))
	{
		change_anchor(setAnchorCenterTop, vec2(0.5f, 0));
	}
	ImGui::SameLine();
	if (ImGui::Button("Right Top", button_size))
	{
		change_anchor(setAnchorRightTop, vec2(1.0f, 0));
	}
	if (ImGui::Button("Left Middle", button_size))
	{
		change_anchor(setAnchorLeftMiddle, vec2(0, 0.5f));
	}
	ImGui::SameLine();
	if (ImGui::Button("Center Middle", button_size))
	{
		change_anchor(setAnchorCenterMiddle, vec2(0.5f, 0.5f));
	}
	ImGui::SameLine();
	if (ImGui::Button("Right Middle", button_size))
	{
		change_anchor(setAnchorRightMiddle, vec2(1.0f, 0.5f));
	}
	if (ImGui::Button("Left Bottom", button_size))
	{
		change_anchor(setAnchorLeftBottom, vec2(0, 1.0f));
	}
	ImGui::SameLine();
	if (ImGui::Button("Center Bottom", button_size))
	{
		change_anchor(setAnchorCenterBottom, vec2(0.5f, 1.0f));
	}
	ImGui::SameLine();
	if (ImGui::Button("Right Bottom", button_size))
	{
		change_anchor(setAnchorRightBottom, vec2(1.0f, 1.0f));
	}

	ImGui::Separator();
	if (ImGui::Button("Expand Width", button_size))
	{
		for (int i = 0; i < elements.size(); i++)
		{
			auto &e = elements[i];
			vec4 p = e->pos;
			vec4 a = e->anchor;
			e->pos = vec4(0, p.y, 0, p.w);
			e->setAnchor(vec4(0, a.y, 1, a.w));	   // calls arrange inside
		}
		c = r = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Expand Height", button_size))
	{
		for (int i = 0; i < elements.size(); i++)
		{
			auto &e = elements[i];
			vec4 p = e->pos;
			vec4 a = e->anchor;
			e->pos = vec4(p.x, 0, p.z, 0);
			e->setAnchor(vec4(a.x, 0, a.z, 1));	   // calls arrange inside
		}
		c = r = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Expand All", button_size))
	{
		for (int i = 0; i < elements.size(); i++)
		{
			auto &e = selected_elements[i];
			e->pos = vec4(0, 0, 0, 0);
			e->setAnchor(vec4(0, 0, 1, 1));	   // calls arrange inside
		}
		c = r = true;
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();

#undef change_anchor

	show_field_int("Order Offset", getOrderOffset, setOrderOffset);
	if (ImGui::IsItemHovered() && elements.size() == 1)
		ImGui::SetTooltip(
			"%s", String::format("Absolute Order: %d", elements[0]->getOrder()).get());
}

void UIDesigner::render_derived_parameters(
	const Unigine::Vector<UI::ElementPtr> &in_elements, bool &c, bool &r)
{
	bool all_the_same = true;
	PropertyPtr prop = in_elements[0]->getProperty()->getParent();
	for (int i = 1; i < in_elements.size(); i++)
	{
		if (prop != in_elements[i]->getProperty()->getParent())
		{
			all_the_same = false;
			break;
		}
	}
	if (!all_the_same)
		return;

	PropertyParameterPtr ref = in_elements[0]->getProperty()->getParameterPtr();
	for (int l = 0; l < ref->getNumChildren(); l++)
	{
		VectorStack<PropertyParameterPtr> elements;
		for (int j = 0; j < in_elements.size(); j++)
			elements.append(in_elements[j]->getProperty()->getParameterPtr()->getChild(l));

		if (elements[0]->isHidden())
			continue;

		StringStack<> name = elements[0]->getTitle();
		if (name.empty())
			name = elements[0]->getName();
		if (name == "Position" || name == "Pivot" || name == "Anchor" || name == "Order")
			continue;

		int type = elements[0]->getType();
		switch (type)
		{
		case Property::PARAMETER_INT:
			show_field_int(name, getValueInt, setValueInt);
			break;
		case Property::PARAMETER_FLOAT:
			if (name.contains("angle", false) || name.contains("rotation", false))
			{
				show_field_float_angle(name, getValueFloat, setValueFloat);
			}
			else
			{
				show_field_float(name, getValueFloat, setValueFloat);
			}
			break;
		case Property::PARAMETER_DOUBLE:
			show_field_double(name, getValueDouble, setValueDouble);
			break;
		case Property::PARAMETER_TOGGLE:
			show_field_bool(name, getValueToggle, setValueToggle);
			break;
		case Property::PARAMETER_SWITCH: {
			size_t num_chars = 0;
			for (int k = 0; k < elements[0]->getSwitchNumItems(); k++)
				num_chars += strlen(elements[0]->getSwitchItemName(k)) + 1;

			String items;
			items.resize(static_cast<int>(num_chars));

			int index = 0;
			for (int k = 0; k < elements[0]->getSwitchNumItems(); k++)
			{
				const char *item_name = elements[0]->getSwitchItemName(k);
#ifdef _LINUX
				strcpy(&(items[index]), item_name);
#else
				strcpy_s(&(items[index]), items.size(), item_name);
#endif
				index += static_cast<int>(strlen(item_name));
				items[index] = '\0';
				index++;
			}

			show_field_combo(name, getValueSwitch, setValueSwitch, int, items);
			break;
		}
		case Property::PARAMETER_STRING:
			show_field_string(name, getValueString, setValueString);
			break;
		case Property::PARAMETER_COLOR:
			show_field_color(name, getValueColor, setValueColor);
			break;
		case Property::PARAMETER_VEC2:
			show_field_vec2(name, getValueVec2, setValueVec2);
			break;
		case Property::PARAMETER_VEC3:
			show_field_vec3(name, getValueVec3, setValueVec3);
			break;
		case Property::PARAMETER_VEC4:
			show_field_vec4(name, getValueVec4, setValueVec4);
			break;
		case Property::PARAMETER_DVEC2:
			// show_field_dvec2(name, getValueDVec2, setValueDVec2);
			break;
		case Property::PARAMETER_DVEC3:
			// show_field_dvec3(name, getValueDVec3, setValueDVec3);
			break;
		case Property::PARAMETER_DVEC4:
			// show_field_dvec4(name, getValueDVec4, setValueDVec4);
			break;
		case Property::PARAMETER_IVEC2:
			show_field_ivec2(name, getValueIVec2, setValueIVec2);
			break;
		case Property::PARAMETER_IVEC3:
			show_field_ivec3(name, getValueIVec3, setValueIVec3);
			break;
		case Property::PARAMETER_IVEC4:
			show_field_ivec4(name, getValueIVec4, setValueIVec4);
			break;
		case Property::PARAMETER_MASK:
			show_field_int(name, getValueMask, setValueMask);
			break;
		case Property::PARAMETER_FILE:
			show_field_string_image(name, getValueFile, setValueFile);
			break;
		case Property::PARAMETER_PROPERTY:
			break;
		case Property::PARAMETER_MATERIAL:
			show_field_material(name, getValueMaterial, setValueMaterial);
			break;
		case Property::PARAMETER_NODE:
			break;
		case Property::PARAMETER_CURVE2D:
			break;
		case Property::PARAMETER_ARRAY:
			break;
		case Property::PARAMETER_STRUCT:
			break;
		}
	}

	render_sprite_shader_parameters(in_elements, c, r);

	if (c || r)
	{
		for (int i = 0; i < in_elements.size(); i++)
			in_elements[i]->applyPropertyChanges();
	}
}

void UIDesigner::render_sprite_shader_parameters(
	const Unigine::Vector<UI::ElementPtr> &in_elements, bool &c, bool &r)
{
	auto shader = dynamic_cast<UI::SpriteShader *>(in_elements[0].get());
	if (!shader || !shader->getMaterial())
		return;

	VectorStack<MaterialPtr> elements;
	for (int i = 0; i < in_elements.size(); i++)
		elements.append(dynamic_cast<UI::SpriteShader *>(in_elements[i].get())->getMaterial());

	MaterialPtr ref = shader->getMaterial();
	for (int i = 0; i < ref->getNumParameters(); i++)
	{
		const char *name = ref->getParameterName(i);
		int type = ref->getParameterType(i);
		switch (type)
		{
		case Material::PARAMETER_FLOAT:
			show_field_float_i(name, getParameterFloat, setParameterFloat, i);
			break;
		case Material::PARAMETER_FLOAT2:
			show_field_vec2_i(name, getParameterFloat2, setParameterFloat2, i);
			break;
		case Material::PARAMETER_FLOAT3:
			show_field_vec3_i(name, getParameterFloat3, setParameterFloat3, i);
			break;
		case Material::PARAMETER_FLOAT4:
			show_field_vec4_i(name, getParameterFloat4, setParameterFloat4, i);
			break;
		case Material::PARAMETER_INT:
			show_field_int_i(name, getParameterInt, setParameterInt, i);
			break;
		case Material::PARAMETER_INT2:
			show_field_ivec2_i(name, getParameterInt2, setParameterInt2, i);
			break;
		case Material::PARAMETER_INT3:
			show_field_ivec3_i(name, getParameterInt3, setParameterInt3, i);
			break;
		case Material::PARAMETER_INT4:
			show_field_ivec4_i(name, getParameterInt4, setParameterInt4, i);
			break;
		default:
			break;
		}
	}
}

void UIDesigner::select_element_by_user(const UI::ElementPtr &element, bool update_hierarchy_window)
{
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL))
	{
		// add/remove selection
		Vector<UI::ElementPtr> new_selection = selected_elements;
		if (new_selection.contains(element))
			new_selection.removeOne(element);
		else
			new_selection.append(element);
		undo_manager->push(new SelectWidgetCommand(selected_elements, new_selection));
		select_elements(new_selection, update_hierarchy_window);
	}
	else if (Input::isKeyPressed(Input::KEY_ANY_SHIFT))
	{
		// add selection
		if (!selected_elements.contains(element))
		{
			Vector<UI::ElementPtr> new_selection = selected_elements;
			new_selection.append(element);
			undo_manager->push(new SelectWidgetCommand(selected_elements, new_selection));
			select_elements(new_selection, update_hierarchy_window);
		}
	}
	else
	{
		// select this only
		undo_manager->push(new SelectWidgetCommand(selected_elements, element));
		select_element(element, update_hierarchy_window);
	}
}

void UIDesigner::select_element(const UI::ElementPtr &element, bool update_hierarchy_window)
{
	// clear focus from any input fields in parameter window
	// to prevent from duplicating values to new selection
	skip_parameter_window = true;

	selected_elements.clear();
	if (element)
		selected_elements.append(element);
	refresh_selected_elements_root();

	// undo/redo
	refresh_selected_element_changes();

	// update hierarchy tree (open and select element)
	if (update_hierarchy_window)
	{
		refresh_selected_elements_parents();
		need_to_focus = true;
	}

#ifdef EDITOR_PLUGIN
	if (sync_selection)
	{
		editor_selected_nodes.clear();
		if (element)
			editor_selected_nodes.append(element->getNode());
		if (editor_selected_nodes.size())
		{
			UnigineEditor::SelectionAction::applySelection(
				UnigineEditor::SelectorNodes::createObjectsSelector(editor_selected_nodes));
		}
		else
		{
			UnigineEditor::Selection::setSelector(
				UnigineEditor::SelectorNodes::createObjectsSelector({}));
		}
	}
#endif
}

void UIDesigner::select_elements(
	const Unigine::Vector<UI::ElementPtr> &elements, bool update_hierarchy_window)
{
	// clear focus from any input fields in parameter window
	// to prevent from duplicating values to new selection
	skip_parameter_window = true;

	sort_and_select_elements(elements);
	refresh_selected_elements_root();

	// undo/redo
	refresh_selected_element_changes();

	// update hierarchy tree (open and select element)
	if (update_hierarchy_window)
	{
		refresh_selected_elements_parents();
		need_to_focus = true;
	}

#ifdef EDITOR_PLUGIN
	if (sync_selection)
	{
		editor_selected_nodes.clear();
		for (int i = 0; i < elements.size(); i++)
			if (elements[i])
				editor_selected_nodes.append(elements[i]->getNode());
		if (editor_selected_nodes.size())
		{
			UnigineEditor::SelectionAction::applySelection(
				UnigineEditor::SelectorNodes::createObjectsSelector(editor_selected_nodes));
		}
		else
		{
			UnigineEditor::Selection::setSelector(
				UnigineEditor::SelectorNodes::createObjectsSelector({}));
		}
	}
#endif
}

void UIDesigner::sort_and_select_elements(const Unigine::Vector<UI::ElementPtr> &elements)
{
	// description: sort elements by hierarchy depth (count of parents)
	// example: rootA, rootC, rootB, parentB, parentA, childB, childA, childD, ...

	if (!elements.size())
	{
		selected_elements.clear();
		return;
	}

	auto get_depth = [](UI::Element *element) {
		int d = 0;
		while (element && element->getParent() != nullptr)
		{
			d++;
			element = element->getParent();
		}
		return d;
	};

	// calc depth of all elements
	Vector<ElementDepth> depths(elements.size());
	for (int i = 0; i < elements.size(); i++)
	{
		ElementDepth &e = depths[i];
		e.element = elements[i];
		e.depth = get_depth(e.element.get());
	}

	// sort depths in ascending order
	quickSort(depths.begin(), depths.end(),
		[](const ElementDepth &a, const ElementDepth &b) { return a.depth < b.depth; });

	// select sorted elements!
	selected_elements.clear();
	for (int i = 0; i < depths.size(); i++)
		selected_elements.append(depths[i].element);
}

void UIDesigner::refresh_selected_elements_root()
{
	selected_roots = selected_elements;

	// a little optimization
	if (selected_roots.size() == 1)
		return;

	// find main elements and remove common children
	for (int i = selected_roots.size() - 1; i >= 0; --i)
	{
		UI::Element *e = selected_roots[i]->getParent();
		while (e)
		{
			int index = selected_roots.findIndex(e);
			if (index >= 0)
			{
				selected_roots.remove(i);
				break;
			}
			e = e->getParent();
		}
	}
}

void UIDesigner::refresh_selected_elements_parents()
{
	selected_parents.clear();
	for (int i = 0; i < selected_elements.size(); i++)
	{
		UI::Element *e = selected_elements[i].get();
		while (e)
		{
			selected_parents.appendUnique(e->getPtr());
			e = e->getParent();
		}
	}
}

Unigine::Math::vec2 UIDesigner::norm_to_window(const Unigine::Math::vec2 &norm_pos) const
{
	// converter from normalized position to window coordinates (Editor's window)
	if (gui_sprite)
	{
		return vec2(gui_sprite->getPositionX() + norm_pos.x * gui_sprite->getWidth(),
			gui_sprite->getPositionY() + norm_pos.y * gui_sprite->getHeight());
	}
	else
		return vec2(norm_pos.x * window_size.x, norm_pos.y * window_size.y);
}

Unigine::Math::vec2 UIDesigner::window_to_screen(const Unigine::Math::vec2 &pos) const
{
	// converter from window coordinates (Editor's window) to canvas's render
	// (returns [0,0] - [render_width, render_height]
	if (gui_sprite)
	{
		float norm_x = (pos.x - gui_sprite->getPositionX()) / gui_sprite->getWidth();
		float norm_y = (pos.y - gui_sprite->getPositionY()) / gui_sprite->getHeight();
		return vec2(norm_x * render_width, norm_y * render_height);
	}
	return pos * dpi_scale;
}

Unigine::Math::vec4 UIDesigner::window_to_screen(const Unigine::Math::vec4 &rect) const
{
	// converter from window coordinates (Editor's window) to canvas's render
	// (returns [0,0] - [render_width, render_height]
	if (gui_sprite)
	{
		int sx = gui_sprite->getPositionX();
		int sy = gui_sprite->getPositionY();
		int sw = gui_sprite->getWidth();
		int sh = gui_sprite->getHeight();

		return vec4(render_width * (rect.x - sx) / sw, render_height * (rect.y - sy) / sh,
			render_width * (rect.z - sx) / sw, render_height * (rect.w - sy) / sh);
	}
	return rect * dpi_scale;
}

Unigine::Math::vec2 UIDesigner::canvas_to_screen(const Unigine::Math::vec2 &pos) const
{
	if (!canvas)
		return pos;

	return pos / canvas->getCanvasPixelSize();
}

Unigine::Math::vec2 UIDesigner::screen_to_canvas(const Unigine::Math::vec2 &pos) const
{
	if (!canvas)
		return pos;

	return pos * canvas->getCanvasPixelSize();
}

Unigine::Math::vec2 UIDesigner::window_to_canvas(const Unigine::Math::vec2 &pos) const
{
	if (!canvas)
		return pos;

	float px = canvas->getCanvasPixelSize();
	if (gui_sprite)
	{
		float norm_x = (pos.x - gui_sprite->getPositionX()) / gui_sprite->getWidth();
		float norm_y = (pos.y - gui_sprite->getPositionY()) / gui_sprite->getHeight();
		return vec2(norm_x * render_width * px, norm_y * render_height * px);
	}
	else
		return pos * dpi_scale * px;
}

Unigine::Math::vec2 UIDesigner::canvas_to_window(const Unigine::Math::vec2 &pos) const
{
	// convert canvas to screen
	vec2 screen_pos = canvas ? pos / canvas->getCanvasPixelSize() : pos;

	// convert screen to window
	if (gui_sprite)
	{
		return vec2(
			gui_sprite->getPositionX() + screen_pos.x * gui_sprite->getWidth() / render_width,
			gui_sprite->getPositionY() + screen_pos.y * gui_sprite->getHeight() / render_height);
	}
	return screen_pos;
}

Unigine::Math::vec2 UIDesigner::canvas_to_norm(const Unigine::Math::vec2 &pos) const
{
	if (!canvas)
		return vec2(pos.x / window_size.x, pos.y / window_size.y);

	return vec2(pos.x / canvas->getCanvasWidth(), pos.y / canvas->getCanvasHeight());
}

float UIDesigner::convert_window_to_canvas(float window_pos) const
{
	// convert window to screen
	float screen_pos =
		gui_sprite ? render_height * window_pos / gui_sprite->getHeight() : window_pos;
	// convert screen to canvas
	return canvas ? screen_pos * canvas->getCanvasPixelSize() : screen_pos;
}

float UIDesigner::convert_canvas_to_window(float canvas_pos) const
{
	// convert canvas to screen
	float screen_pos = canvas ? canvas_pos / canvas->getCanvasPixelSize() : canvas_pos;

	// convert screen to window
	return gui_sprite ? screen_pos * gui_sprite->getHeight() / render_height : screen_pos;
}

void UIDesigner::create_canvas_node()
{
	prev_selected_elements = selected_elements;

	// hide previous canvases
	auto &all_canvases = UI::Canvas::get()->getAllCanvases();
	for (int i = 0; i < all_canvases.size(); i++)
		all_canvases[i]->getGui()->setHidden(true);

	// create new canvas
	NodeDummyPtr canvas_node = NodeDummy::create();
	canvas_node->setName("canvas");
	canvas_node->setShowInEditorEnabled(true);
	canvas_node->setSaveToWorldEnabled(true);
	UI::Canvas *c = ComponentSystem::get()->addComponent<UI::Canvas>(canvas_node);
	canvas = c->getPtr();
	select_element(canvas);
	undo_manager->push(new CreatedWidgetCommand(prev_selected_elements, selected_elements));
}

void UIDesigner::find_all_creator_elements()
{
	all_creator_elements.clear();

	// add default elements in frequency order
	all_creator_elements.append("UI_Canvas");
	all_creator_elements.append("UI_Element");
	all_creator_elements.append("UI_Label");
	all_creator_elements.append("UI_Sprite");
	all_creator_elements.append("UI_SpriteShader");
	all_creator_elements.append("UI_Button");
	all_creator_elements.append("UI_EditLine");
	all_creator_elements.append("UI_CheckBox");
	all_creator_elements.append("UI_Slider");
	all_creator_elements.append("UI_Scroll");
	all_creator_elements.append("UI_ScrollBox");
	all_creator_elements.append("UI_DrawSurface");
	all_creator_elements.append("UI_ClipMask");
	all_creator_elements.append("UI_Table");
	all_creator_elements.append("UI_Window");

	for (int i = 0; i < Properties::getNumProperties(); i++)
	{
		PropertyPtr prop = Properties::getProperty(i);
		if (prop->isInternal())
			continue;

		StringStack<> name = prop->getName();
		if (name.startsWith("UI_"))
			all_creator_elements.appendUnique(name);
	}

	all_creator_elements.removeOne("UI_Canvas");
}

void UIDesigner::check_node_selection()
{
#ifdef EDITOR_PLUGIN
	if (sync_selection)
	{
		// get current selection
		VectorStack<NodePtr> cur_selection;
		if (UnigineEditor::Selection::instance())
		{
			const UnigineEditor::SelectorNodes *snodes =
				UnigineEditor::Selection::getSelectorNodes();
			if (snodes)
				cur_selection = snodes->getNodes();
		}

		if (editor_selected_nodes != cur_selection)
		{
			editor_selected_nodes = cur_selection;

			VectorStack<UI::ElementPtr> new_selected_elements;
			for (int i = 0; i < editor_selected_nodes.size(); i++)
			{
				UI::Element *e =
					ComponentSystem::get()->getComponent<UI::Element>(editor_selected_nodes[i]);
				if (e)
					new_selected_elements.append(e->getPtr());
			}
			if (editor_selected_nodes.size() && new_selected_elements.empty())
			{
				// if a user select non-element nodes:
				// 1. clear selection in the editor
				// 2. store user's selected nodes
				sync_selection = false;
			}
			select_elements(new_selected_elements);
			if (editor_selected_nodes.size() && new_selected_elements.empty())
				sync_selection = true;
		}
	}
#endif
}

void UIDesigner::mouse_update()
{
	bool mouse = false;
	if (Engine::get()->isEditorLoaded())
	{
		GuiPtr gui = canvas_widget->getGui();
		int x = gui->getMouseX();
		int y = gui->getMouseY();
		mouse = gui->getMouseButtons() == 1;
		mouse_position = ivec2(x, y);
	}
	else
	{
		auto get_mouse_position = []() -> ivec2 {
			auto window = Unigine::WindowManager::getMainWindow();
			if (!window)
				return Unigine::Math::ivec2_zero;

			Unigine::Math::ivec2 window_pos = window->getClientPosition();
			return Unigine::Input::getMousePosition() - window_pos;
		};

		mouse = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT);
		mouse_position = get_mouse_position();

		if (dpi_scale != 1)
		{
			mouse_position.x = ftoi(itof(mouse_position.x) / dpi_scale);
			mouse_position.y = ftoi(itof(mouse_position.y) / dpi_scale);
		}
	}

	// input
	if (mouse)
	{
		if (!mouse_down && !mouse_hold)
		{
			mouse_down = true;
			mouse_down_pos = mouse_position;
		}
		else
		{
			mouse_down = false;
			mouse_hold = true;
		}
	}
	else
	{
		if (!mouse_up && mouse_hold)
			mouse_up = true;
		else
		{
			mouse_up = false;
			mouse_hold = false;
		}
	}
	if (Console::isActive() || ImGuiImpl::isWantCaptureMouse() || file_dialog_show)
	{
		mouse_down = false;
		mouse_hold = false;
		mouse_up = false;
	}
	if (mouse_hold)
		mouse_hold_pos = mouse_position;

	// grid: get actual grid size for element aligning
	grid = grid_size * (Input::isKeyPressed(Input::KEY_ANY_CTRL) ? grid_multiplier : 1);

	// clear align lines
	canvas_widget->clearLinePoints(14);
	canvas_widget->clearLinePoints(15);

	// react on mouse button down/hold/up in the canvas
	if (is_selection_mode())
		selection_mode_update();
	else
		creation_mode_update();
}

void UIDesigner::mouse_context_menu_update()
{
	static bool show_popup = false;
	static ivec2 mouse_rmb_pos;

	// don't show context menu if mouse position is outside the window
	if (!show_popup
		&& (mouse_position.x < 0 || mouse_position.y < 0 || mouse_position.x > window_size.x
			|| mouse_position.y > window_size.y))
		return;

	if (ImGui::BeginPopupContextVoid("ui_context"))
	{
		if (!canvas)
		{
			if (ImGui::MenuItem("Create Canvas"))
			{
				create_canvas_node();
			}
			ImGui::EndPopup();	  // BeginPopupContextVoid
			return;
		}

		// -----------------------------------------------------------------------

		if (!show_popup)
		{
			show_popup = true;
			mouse_rmb_pos = mouse_position;
		}

		if (ImGui::BeginMenu("Create"))
		{
			for (int i = 0; i < all_creator_elements.size(); i++)
			{
				if (ImGui::MenuItem(all_creator_elements[i].get() + 3))
				{
					float mouse_x = itof(mouse_rmb_pos.x);
					float mouse_y = itof(mouse_rmb_pos.y);
					mouse_start_pos = vec2(mouse_x, mouse_y);

					prev_selected_elements = selected_elements;

					NodeDummyPtr node = NodeDummy::create();
					node->setShowInEditorEnabled(true);
					node->setSaveToWorldEnabled(true);
					node->setParent(canvas->getNode());
					node->setName(all_creator_elements[i].get() + 3);
					node->addProperty(all_creator_elements[i]);
					UI::ElementPtr new_element =
						ComponentSystem::get()->getComponent<UI::Element>(node)->getPtr();
					new_element->initializeElement();
					vec4 pos = new_element->pos;
					pos.xy = window_to_canvas(mouse_start_pos);
					if (dynamic_cast<UI::ElementFocusable *>(new_element.get())
						|| dynamic_cast<UI::ProgressBar *>(new_element.get()))
					{
						pos.z = 512;
						pos.w = 40;
					}
					else
					{
						pos.z = 256;
						pos.w = 256;
					}
					new_element->pos = pos;
					select_element(new_element);
					undo_manager->push(
						new CreatedWidgetCommand(prev_selected_elements, selected_elements));
					selected_axis = AXIS::NONE;
					mouse_grab_type = GRAB_TYPE::VERTEX_RB;

					last_manipulator_p0 = mouse_start_pos;
					last_manipulator_p2 =
						mouse_start_pos
						+ vec2(convert_canvas_to_window(pos.z), convert_canvas_to_window(pos.w));
					last_manipulator_aspect =
						new_element->getHeight() != 0
							? new_element->getWidth() / new_element->getHeight()
							: 1.0f;
				}
			}
			ImGui::EndMenu();
		}

		auto save_changes_to_undo_redo = [this]() {
			selected_element_changes->saveState();
			undo_manager->push(selected_element_changes);
			selected_element_changes = nullptr;
			refresh_selected_element_changes();
		};

		if (selected_elements.size())
		{
			ImGui::Separator();
			if (ImGui::BeginMenu("Set Pivot"))
			{
				static bool recalc_position = true;
				ImGui::Checkbox("Recalculate Positions##ctx", &recalc_position);

				ImVec2 button_size = ImVec2(80.0f, 0.0f);

				auto change_pivot = [this, recalc_position = recalc_position](const vec2 &pivot) {
					for (int i = 0; i < selected_elements.size(); i++)
					{
						auto &e = selected_elements[i];
						vec4 pos = recalc_position ? e->getWorldPosition() : vec4_zero;

						e->setPivot(pivot);

						if (recalc_position)
							e->setWorldPosition(pos);
					}
					ImGui::CloseCurrentPopup();
				};

				if (ImGui::Button("Left Top", button_size))
				{
					change_pivot(vec2(0, 0));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Center Top", button_size))
				{
					change_pivot(vec2(0.5f, 0));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Right Top", button_size))
				{
					change_pivot(vec2(1.0f, 0));
					save_changes_to_undo_redo();
				}
				if (ImGui::Button("Left Middle", button_size))
				{
					change_pivot(vec2(0, 0.5f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Center Middle", button_size))
				{
					change_pivot(vec2(0.5f, 0.5f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Right Middle", button_size))
				{
					change_pivot(vec2(1.0f, 0.5f));
					save_changes_to_undo_redo();
				}
				if (ImGui::Button("Left Bottom", button_size))
				{
					change_pivot(vec2(0, 1.0f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Center Bottom", button_size))
				{
					change_pivot(vec2(0.5f, 1.0f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Right Bottom", button_size))
				{
					change_pivot(vec2(1.0f, 1.0f));
					save_changes_to_undo_redo();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Set Anchor"))
			{
				static bool recalc_position = true;
				static bool change_pivot_too = false;
				ImGui::Checkbox("Recalculate Positions##ctx2", &recalc_position);
				ImGui::SameLine();
				ImGui::Checkbox("Change Pivot too##ctx2", &change_pivot_too);

				ImVec2 button_size = ImVec2(80.0f, 0.0f);

				auto change_anchor = [this, change_pivot_too = change_pivot_too,
										 recalc_position = recalc_position](
										 const vec4 &anchor, const vec2 &pivot) {
					for (int i = 0; i < selected_elements.size(); i++)
					{
						auto &e = selected_elements[i];
						vec4 pos = recalc_position ? e->getWorldPosition() : vec4_zero;

						e->setAnchor(anchor);
						if (change_pivot_too)
							e->setPivot(pivot);

						if (recalc_position)
							e->setWorldPosition(pos);
					}
					ImGui::CloseCurrentPopup();
				};

				if (ImGui::Button("Left Top", button_size))
				{
					change_anchor(vec4(0, 0, 0, 0), vec2(0, 0));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Center Top", button_size))
				{
					change_anchor(vec4(0.5f, 0.0f, 0.5f, 0.0f), vec2(0.5f, 0));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Right Top", button_size))
				{
					change_anchor(vec4(1, 0, 1, 0), vec2(1.0f, 0));
					save_changes_to_undo_redo();
				}
				if (ImGui::Button("Left Middle", button_size))
				{
					change_anchor(vec4(0.0f, 0.5f, 0.0f, 0.5f), vec2(0, 0.5f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Center Middle", button_size))
				{
					change_anchor(vec4(0.5f, 0.5f, 0.5f, 0.5f), vec2(0.5f, 0.5f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Right Middle", button_size))
				{
					change_anchor(vec4(1.0f, 0.5f, 1.0f, 0.5f), vec2(1.0f, 0.5f));
					save_changes_to_undo_redo();
				}
				if (ImGui::Button("Left Bottom", button_size))
				{
					change_anchor(vec4(0, 1, 0, 1), vec2(0, 1.0f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Center Bottom", button_size))
				{
					change_anchor(vec4(0.5f, 1.0f, 0.5f, 1.0f), vec2(0.5f, 1.0f));
					save_changes_to_undo_redo();
				}
				ImGui::SameLine();
				if (ImGui::Button("Right Bottom", button_size))
				{
					change_anchor(vec4(1, 1, 1, 1), vec2(1.0f, 1.0f));
					save_changes_to_undo_redo();
				}
				ImGui::Separator();
				if (ImGui::Button("Expand Width", button_size))
				{
					for (int i = 0; i < selected_elements.size(); i++)
					{
						auto &e = selected_elements[i];
						vec4 p = e->pos;
						vec4 a = e->anchor;
						e->pos = vec4(0, p.y, 0, p.w);
						e->setAnchor(vec4(0, a.y, 1, a.w));	   // calls arrange inside
					}
					save_changes_to_undo_redo();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Expand Height", button_size))
				{
					for (int i = 0; i < selected_elements.size(); i++)
					{
						auto &e = selected_elements[i];
						vec4 p = e->pos;
						vec4 a = e->anchor;
						e->pos = vec4(p.x, 0, p.z, 0);
						e->setAnchor(vec4(a.x, 0, a.z, 1));	   // calls arrange inside
					}
					save_changes_to_undo_redo();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Expand All", button_size))
				{
					for (int i = 0; i < selected_elements.size(); i++)
					{
						auto &e = selected_elements[i];
						e->pos = vec4(0, 0, 0, 0);
						e->setAnchor(vec4(0, 0, 1, 1));	   // calls arrange inside
					}
					save_changes_to_undo_redo();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Cut", "Ctrl+X", false, selected_elements.size() > 0))
			cut_selected();
		if (ImGui::MenuItem("Copy", "Ctrl+C", false, selected_elements.size() > 0))
			copy_to_clipboard();
		if (ImGui::MenuItem("Paste", "Ctrl+V", false, clipboard.size() > 0))
			paste_from_clipboard();
		if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selected_elements.size() > 0))
			duplicate_selected();
		if (ImGui::MenuItem("Delete", "Del", false, selected_elements.size() > 0))
			destroy_selected();

		ImGui::EndPopup();	  // BeginPopupContextVoid
	}
	else
		show_popup = false;
}

void UIDesigner::keyboard_update()
{
	if (Console::isActive() || ImGuiImpl::isWantCaptureKeyboard())
		return;

	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_Z))
		undo_manager->undo();
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_Y))
		undo_manager->redo();

	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_X))
		cut_selected();
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_C))
		copy_to_clipboard();
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_V))
		paste_from_clipboard();
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_D))
		duplicate_selected();
	if (Input::isKeyDown(Input::KEY_DELETE))
		destroy_selected();

	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_A))
		select_all();
	if (Input::isKeyPressed(Input::KEY_ANY_ALT) && Input::isKeyDown(Input::KEY_A))
		deselect_all();

#ifdef EDITOR_PLUGIN
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL) && Input::isKeyDown(Input::KEY_S))
		World::saveWorld();
#endif

	move_element_by_keyboard();
}

bool UIDesigner::is_selection_mode()
{
	return selected_creator.empty();
}

void UIDesigner::selection_mode_update()
{
	bool want_add_or_remove =
		Input::isKeyPressed(Input::KEY_ANY_CTRL) || Input::isKeyPressed(Input::KEY_ANY_SHIFT);

	// select some widget or start using rectange selection
	if (mouse_down)
	{
		// init
		float mouse_x = itof(mouse_down_pos.x);
		float mouse_y = itof(mouse_down_pos.y);
		mouse_start_pos = vec2(mouse_x, mouse_y);
		selected_axis = AXIS::NONE;

		// manipulator is shown
		// user can select corners or edges of this manipulator... check it!
		Vector<UI::ElementPtr> prev_selected = selected_elements;
		if (selected_elements.size() > 0)
		{
			if (!update_manipulator_select(mouse_start_pos, manipulator))
			{
				// user click on another widget or empty space!
				if (!want_add_or_remove)
					clear_selection();
			}
			else if (selected_elements.size() == 1 && selected_elements[0] == canvas)
			{
				// special case for canvases
				mouse_grab_type = GRAB_TYPE::NONE;
				clear_selection();
			}
		}

		// manipulator isn't shown
		// user can click on any widget and move it everywhere...
		// ...or user can select multiple widgets by rectangle selection
		if (selected_elements.size() == 0 || want_add_or_remove)
		{
			// find widget under mouse cursor (latest created item)
			mouse_grab_type = GRAB_TYPE::NONE;
			vec2 ms = window_to_screen(vec2(mouse_x, mouse_y));
			UI::ElementPtr hover_element = find_element(ms.x, ms.y);
			if (hover_element)	  // found!
			{
				select_element_by_user(hover_element, true);

				// user maybe want to moving this widget at the "mouse_hold" stage
				if (!want_add_or_remove)
					mouse_grab_type = GRAB_TYPE::PLANE;
			}
		}

		// preparing selected widget for "mouse_hold" stage
		if (selected_elements.size() > 0)
		{
			widget_canvas_start_pos.clear();
			widget_screen_start_size.clear();
			widget_start_aspect.clear();
			widget_norm_pos.clear();
			widget_local_bounds.clear();

			float manipulator_width = manipulator.p2.x - manipulator.p0.x;
			float manipulator_height = manipulator.p2.y - manipulator.p0.y;
			for (int i = 0; i < selected_elements.size(); i++)
			{
				const UI::ElementPtr &s = selected_elements[i];

				widget_canvas_start_pos.append(
					vec4(s->getPositionX(), s->getPositionY(), s->getWidth(), s->getHeight()));
				widget_screen_start_size.append(ivec2(get_width(s), get_height(s)));
				widget_start_aspect.append(
					itof(widget_screen_start_size.last().x) / widget_screen_start_size.last().y);

				// local boundaries in window coordinates
				vec2 min_s = norm_to_window(s->getNormalizedBoundMin());
				vec2 max_s = norm_to_window(s->getNormalizedBoundMax());

				// find normalized coordinates (corners)
				vec4 np;
				int spx = get_screen_position_x(s);
				int spy = get_screen_position_y(s);
				vec2 m0 = window_to_screen(min_s);
				vec2 m2 = window_to_screen(max_s);
				np.x = (spx - m0.x) / (m2.x - m0.x);
				np.y = (spy - m0.y) / (m2.y - m0.y);
				np.z = (spx - m0.x + get_width(s)) / (m2.x - m0.x);
				np.w = (spy - m0.y + get_height(s)) / (m2.y - m0.y);
				widget_norm_pos.append(np);

				// calc local boundaries (manipulators) relative to full manipulator
				vec4 lb = vec4(min_s.x, min_s.y, max_s.x, max_s.y);
				lb.x = (lb.x - manipulator.p0.x)
					   / manipulator_width;	   // normalize relative to "manipulator"
				lb.y = (lb.y - manipulator.p0.y) / manipulator_height;
				lb.z = (lb.z - manipulator.p0.x) / manipulator_width;
				lb.w = (lb.w - manipulator.p0.y) / manipulator_height;
				widget_local_bounds.append(lb);
			}
			if (selected_elements.size() == 1)
				widget_local_bounds[0] = vec4(0, 0, 1, 1);

			last_manipulator_p0 = manipulator.p0;
			last_manipulator_p2 = manipulator.p2;
			last_manipulator_aspect =
				(manipulator.p2.x - manipulator.p0.x) / (manipulator.p2.y - manipulator.p0.y);
		}

		find_all_snappable_elements();
	}

	// move some widget or using rectangle selection
	if (mouse_hold)
	{
		// mouse info
		vec2 mouse_pos = vec2(itof(mouse_hold_pos.x), itof(mouse_hold_pos.y));
		vec2 mouse_offset = (mouse_pos - mouse_start_pos) * dpi_scale;
		ivec2 imouse_offset = ivec2(ftoi(mouse_offset.x), ftoi(mouse_offset.y));

		// if mouse was moved
		if (imouse_offset.x != 0 || imouse_offset.y != 0)
		{
			// using rectangle selection
			if (mouse_grab_type == GRAB_TYPE::NONE)
			{
				clear_selection();

				// get rectangle corners
				vec4 selection_rectangle(itof(min(mouse_down_pos.x, mouse_hold_pos.x)),
					itof(min(mouse_down_pos.y, mouse_hold_pos.y)),
					itof(max(mouse_down_pos.x, mouse_hold_pos.x)),
					itof(max(mouse_down_pos.y, mouse_hold_pos.y)));

				// find widgets under rectangle and add it to the list
				selection_rectangle = window_to_screen(selection_rectangle);
				Vector<UI::ElementPtr> hover_elements = find_elements(selection_rectangle);
				if (hover_elements.size())
				{
					// found! add this to our selection!
					select_elements(hover_elements);
				}
			}
			// move the selected widget
			else
			{
				float aspect = last_manipulator_aspect;
				int shift = Input::isKeyPressed(Input::KEY_ANY_SHIFT);
				int selection_count =
					(mouse_grab_type == GRAB_TYPE::PLANE ? selected_roots.size()
														 : selected_elements.size());

				for (int i = 0; i < selection_count; i++)
				{
					const UI::ElementPtr &selected_widget =
						(mouse_grab_type == GRAB_TYPE::PLANE ? selected_roots[i]
															 : selected_elements[i]);
					const UI::ICanvas *c = selected_widget->getCanvas();

					// Q: why we check GrabType::PLANE?
					// A: we don't want offset children in our selection. Only parents.

					// move
					if (mouse_grab_type == GRAB_TYPE::PLANE)
					{
						if (shift && selected_axis == AXIS::NONE
							&& length(mouse_offset) > select_axis_threshold)
						{
							if (Math::abs(imouse_offset.x) > Math::abs(imouse_offset.y))
								selected_axis = AXIS::HORIZONTAL;
							else
								selected_axis = AXIS::VERTICAL;
						}

						// set new position
						float px = widget_canvas_start_pos[i].x;
						float py = widget_canvas_start_pos[i].y;
						float dx, dy;
						if (gui_sprite)
						{
							// convert window to screen
							float screen_dx =
								render_height * itof(imouse_offset.x) / gui_sprite->getHeight();
							float screen_dy =
								render_height * itof(imouse_offset.y) / gui_sprite->getHeight();
							// convert screen to canvas
							dx = screen_dx * c->getCanvasPixelSize();
							dy = screen_dy * c->getCanvasPixelSize();
						}
						else
						{
							dx = c->convertScreenToCanvas(imouse_offset.x);
							dy = c->convertScreenToCanvas(imouse_offset.y);
						}
						if (shift && selected_axis != AXIS::NONE)
						{
							if (selected_axis == AXIS::HORIZONTAL)
								dy = 0;
							else if (selected_axis == AXIS::VERTICAL)
								dx = 0;
						}

						vec2 xy = snap(px + dx, py + dy,
							selected_widget->isFixedWidth() ? selected_widget->getWidth() : 0,
							selected_widget->isFixedHeight() ? selected_widget->getHeight() : 0);
						selected_widget->setPosition(xy.x, xy.y);

						// special case: stretched anchors (need to change right/bottom sides
						// also)
						vec4 anc = selected_widget->getAnchor();
						if (anc.x != anc.z
							&& (selected_axis == AXIS::HORIZONTAL || selected_axis == AXIS::NONE))
						{
							selected_widget->setRightOffset(
								widget_canvas_start_pos[i].z - (xy.x - px));
						}
						if (anc.y != anc.w
							&& (selected_axis == AXIS::VERTICAL || selected_axis == AXIS::NONE))
						{
							selected_widget->setBottomOffset(
								widget_canvas_start_pos[i].w - (xy.y - py));
						}
					}
					// scale
					else
					{
						vec2 p0 = window_to_screen(last_manipulator_p0);
						vec2 p2 = window_to_screen(last_manipulator_p2);
						float mo_x, mo_y;
						if (gui_sprite)
						{
							mo_x = render_height * itof(imouse_offset.x) / gui_sprite->getHeight();
							mo_y = render_height * itof(imouse_offset.y) / gui_sprite->getHeight();
						}
						else
						{
							mo_x = itof(imouse_offset.x);
							mo_y = itof(imouse_offset.y);
						}

						// scale by edges
						if (mouse_grab_type == GRAB_TYPE::EDGE_LEFT)
						{
							p0.x += mo_x;
							p2.y += shift ? (-mo_x / aspect) : 0;
						}
						else if (mouse_grab_type == GRAB_TYPE::EDGE_RIGHT)
						{
							p2.x += mo_x;
							p2.y += shift ? (mo_x / aspect) : 0;
						}
						else if (mouse_grab_type == GRAB_TYPE::EDGE_TOP)
						{
							p0.y += mo_y;
							p2.x += shift ? (-mo_y * aspect) : 0;
						}
						else if (mouse_grab_type == GRAB_TYPE::EDGE_BOTTOM)
						{
							p2 += vec2(shift ? (mo_y * aspect) : 0, mo_y);
						}
						// scale by vertices
						else if (mouse_grab_type == GRAB_TYPE::VERTEX_LT)
						{
							p0 += vec2(mo_x, shift ? (mo_x / aspect) : mo_y);
						}
						else if (mouse_grab_type == GRAB_TYPE::VERTEX_RT)
						{
							p2.x += mo_x;
							p0.y += shift ? (-mo_x / aspect) : mo_y;
						}
						else if (mouse_grab_type == GRAB_TYPE::VERTEX_RB)
						{
							p2 += vec2(mo_x, shift ? (mo_x / aspect) : mo_y);
						}
						else if (mouse_grab_type == GRAB_TYPE::VERTEX_LB)
						{
							p0.x += mo_x;
							p2.y += shift ? (-mo_x / aspect) : mo_y;
						}

						// calculate local boundaries
						vec2 pl0, pl2;
						pl0.x = lerp(p0.x, p2.x, widget_local_bounds[i].x);
						pl0.y = lerp(p0.y, p2.y, widget_local_bounds[i].y);
						pl2.x = lerp(p0.x, p2.x, widget_local_bounds[i].z);
						pl2.y = lerp(p0.y, p2.y, widget_local_bounds[i].w);

						// snap
						pl0 = canvas_to_screen(snap(screen_to_canvas(pl0)));
						pl2 = canvas_to_screen(snap(screen_to_canvas(pl2)));

						// set new position and scale
						float px_size = c->getCanvasPixelSize();	// screen to canvas converter
						vec4 anc = selected_widget->getAnchor();

						UI::ElementPtr pe = selected_widget->getParent()->getPtr();
						ivec4 parent =
							pe ? ivec4(get_screen_position_x(pe), get_screen_position_y(pe),
									 get_screen_position_x1(pe), get_screen_position_y1(pe))
							   : ivec4(0, 0, c->getScreenWidth(), c->getScreenHeight());

						vec4 &norm = widget_norm_pos[i];
						float x = (pl0.x - lerp(itof(parent.x), itof(parent.z), anc.x)) * px_size;
						float y = (pl0.y - lerp(itof(parent.y), itof(parent.w), anc.y)) * px_size;
						float w = (norm.z - norm.x) * (pl2.x - pl0.x) * px_size;
						float h = (norm.w - norm.y) * (pl2.y - pl0.y) * px_size;

						// apply pivot
						x += selected_widget->getPivotX() * w;
						y += selected_widget->getPivotY() * h;

						// apply position, width and height (in canvas coords)
						selected_widget->setPosition(Math::round(x), Math::round(y));
						selected_widget->setWidth(Math::round(w));
						selected_widget->setHeight(Math::round(h));

						// what if anchor is non fixed:
						bool stretched_x = anc.x != anc.z;
						bool stretched_y = anc.y != anc.w;
						if (stretched_x)
						{
							x = (pl0.x - lerp(itof(parent.x), itof(parent.z), anc.x)) * px_size;
							w = (lerp(itof(parent.x), itof(parent.z), anc.z) - pl2.x) * px_size;
							selected_widget->setLeftOffset(Math::round(x));
							selected_widget->setRightOffset(Math::round(w));
						}
						if (stretched_y)
						{
							y = (pl0.y - lerp(itof(parent.y), itof(parent.w), anc.y)) * px_size;
							h = (lerp(itof(parent.y), itof(parent.w), anc.w) - pl2.y) * px_size;
							selected_widget->setTopOffset(Math::round(y));
							selected_widget->setBottomOffset(Math::round(h));
						}
					}
				}

				widget_moved = true;
			}
		}
	}

	// circular selection
	if (mouse_up)
	{
		// 1. select one instead of many
		if (selected_elements.size() > 1 && mouse_down_pos == mouse_hold_pos && !want_add_or_remove)
		{
			float mouse_x = itof(mouse_down_pos.x);
			float mouse_y = itof(mouse_down_pos.y);
			vec2 ms = window_to_screen(vec2(mouse_x, mouse_y));
			UI::ElementPtr hover_element = find_element(ms.x, ms.y);
			if (hover_element)
			{
				undo_manager->apply(new SelectWidgetCommand(selected_elements, hover_element));
				mouse_grab_type = GRAB_TYPE::PLANE;
			}
		}

		// 2. select one next
		else if (selected_elements.size() == 1 && prev_selected_elements.size() == 1
				 && prev_selected_elements[0] == selected_elements[0]
				 && mouse_down_pos == mouse_hold_pos)
		{
			float mouse_x = itof(mouse_down_pos.x);
			float mouse_y = itof(mouse_down_pos.y);
			vec2 ms = window_to_screen(vec2(mouse_x, mouse_y));
			UI::ElementPtr hover_element = find_next_element(ms.x, ms.y, selected_elements[0]);
			if (hover_element)
			{
				// found! user will moving this widget at the "mouse_hold" stage
				undo_manager->apply(new SelectWidgetCommand(selected_elements, hover_element));
				mouse_grab_type = GRAB_TYPE::PLANE;
			}
			else
			{
				// deselect
				undo_manager->apply(new SelectWidgetCommand(selected_elements, UI::ElementPtr()));
				mouse_grab_type = GRAB_TYPE::NONE;
			}
		}

		// 3. rectangle selection single element case
		else if (mouse_grab_type == GRAB_TYPE::NONE && prev_selected_elements != selected_elements)
		{
			undo_manager->push(new SelectWidgetCommand(prev_selected_elements, selected_elements));
		}

		// save to undo system widget's transformation
		if (widget_moved && selected_element_changes)
		{
			selected_element_changes->saveState();
			undo_manager->push(selected_element_changes);
			selected_element_changes = nullptr;
			refresh_selected_element_changes();
		}

		prev_selected_elements = selected_elements;
	}
}

void UIDesigner::creation_mode_update()
{
	// create widget
	if (mouse_down)
	{
		float mouse_x = itof(mouse_down_pos.x);
		float mouse_y = itof(mouse_down_pos.y);
		mouse_start_pos = vec2(mouse_x, mouse_y);

		prev_selected_elements = selected_elements;

		NodeDummyPtr node = NodeDummy::create();
		node->setShowInEditorEnabled(true);
		node->setSaveToWorldEnabled(true);
		node->setParent(canvas->getNode());
		node->setName(selected_creator.get() + 3);
		node->addProperty(selected_creator);
		UI::ElementPtr new_element =
			ComponentSystem::get()->getComponent<UI::Element>(node)->getPtr();
		vec4 pos = new_element->pos;
		pos.xy = window_to_canvas(mouse_start_pos);
		new_element->pos = pos;
		select_element(new_element);
		selected_axis = AXIS::NONE;
		mouse_grab_type = GRAB_TYPE::VERTEX_RB;

		last_manipulator_p0 = mouse_start_pos;
		last_manipulator_p2 = mouse_start_pos;
		last_manipulator_aspect = new_element->getHeight() != 0
									  ? new_element->getWidth() / new_element->getHeight()
									  : 1.0f;

		find_all_snappable_elements();
	}

	// resize created widget
	if (mouse_hold)
	{
		// mouse info
		vec2 mouse_pos = vec2(itof(mouse_hold_pos.x), itof(mouse_hold_pos.y));
		vec2 mouse_offset = (mouse_pos - mouse_start_pos) * dpi_scale;
		ivec2 imouse_offset = ivec2(ftoi(mouse_offset.x), ftoi(mouse_offset.y));

		// if mouse was moved
		if (imouse_offset.x != 0 || imouse_offset.y != 0)
		{
			// calc mouse offset in screen coordinates
			vec2 p0 = window_to_screen(last_manipulator_p0);
			vec2 p2 = window_to_screen(last_manipulator_p2);
			float mo_x, mo_y;
			if (gui_sprite)
			{
				mo_x = render_height * itof(imouse_offset.x) / gui_sprite->getHeight();
				mo_y = render_height * itof(imouse_offset.y) / gui_sprite->getHeight();
			}
			else
			{
				mo_x = itof(imouse_offset.x);
				mo_y = itof(imouse_offset.y);
			}

			// scale by right bottom vertex
			float aspect = last_manipulator_aspect;
			int shift = Input::isKeyPressed(Input::KEY_ANY_SHIFT);
			p2 += vec2(mo_x, shift ? (mo_x / aspect) : mo_y);

			// set new scale
			selected_elements[0]->setWorldPosition(
				snap(screen_to_canvas(p0)), snap(screen_to_canvas(p2)));
		}
	}

	// finish creation, added info to undo/redo
	if (mouse_up)
	{
		selected_creator.clear();
		undo_manager->push(new CreatedWidgetCommand(prev_selected_elements, selected_elements));
	}
}

void UIDesigner::check_selection_existence()
{
	for (int i = selected_elements.size() - 1; i >= 0; --i)
	{
		if (!selected_elements[i])
			selected_elements.remove(i);
	}
}

void UIDesigner::update_selection_parameters()
{
#ifdef EDITOR_PLUGIN
	for (int i = 0; i < selected_elements.size(); i++)
	{
		if (selected_elements[i]->isInitialized())
			selected_elements[i]->applyPropertyChanges();
	}
#endif
}

void UIDesigner::update_manipulator()
{
	if (!selected_elements.size())
		return;

	// get bound rectangle of all selected widgets
	vec2 p0(Consts::INF, Consts::INF), p1(Consts::INF, -Consts::INF),
		p2(-Consts::INF, -Consts::INF), p3(-Consts::INF, Consts::INF);
	for (int i = 0; i < selected_elements.size(); i++)
	{
		vec2 min_s = norm_to_window(selected_elements[i]->getNormalizedBoundMin());
		vec2 max_s = norm_to_window(selected_elements[i]->getNormalizedBoundMax());

		vec2 pp0, pp1, pp2, pp3;
		pp0 = vec2(min_s.x, min_s.y);
		pp1 = vec2(min_s.x, max_s.y);
		pp2 = vec2(max_s.x, max_s.y);
		pp3 = vec2(max_s.x, min_s.y);

		p0.x = min(p0.x, pp0.x);
		p0.y = min(p0.y, pp0.y);
		p1.x = min(p1.x, pp1.x);
		p1.y = max(p1.y, pp1.y);
		p2.x = max(p2.x, pp2.x);
		p2.y = max(p2.y, pp2.y);
		p3.x = max(p3.x, pp3.x);
		p3.y = min(p3.y, pp3.y);
	}
	manipulator.p0 = p0;
	manipulator.p1 = p1;
	manipulator.p2 = p2;
	manipulator.p3 = p3;
}

bool UIDesigner::update_manipulator_select(const vec2 &mouse_pos, const Manipulator &manipulator)
{
	return update_manipulator_select(
		mouse_pos, manipulator.p0, manipulator.p1, manipulator.p2, manipulator.p3);
}

bool UIDesigner::update_manipulator_select(
	const vec2 &mouse_pos, const vec2 &p0, const vec2 &p1, const vec2 &p2, const vec2 &p3)
{
	// check vertices
	float d0 = length(mouse_pos - p0);
	float d1 = length(mouse_pos - p1);
	float d2 = length(mouse_pos - p2);
	float d3 = length(mouse_pos - p3);
	if (d0 < vertex_radius || d1 < vertex_radius || d2 < vertex_radius || d3 < vertex_radius)
	{
		if (d0 <= d1 && d0 <= d2 && d0 <= d3)
		{
			mouse_grab_type = GRAB_TYPE::VERTEX_LT;
			return 1;
		}
		else if (d1 <= d0 && d1 <= d2 && d1 <= d3)
		{
			mouse_grab_type = GRAB_TYPE::VERTEX_LB;
			return 1;
		}
		else if (d2 <= d0 && d2 <= d1 && d2 <= d3)
		{
			mouse_grab_type = GRAB_TYPE::VERTEX_RB;
			return 1;
		}
		else
		{
			mouse_grab_type = GRAB_TYPE::VERTEX_RT;
			return 1;
		}
	}
	// check edges
	else
	{
		float e0 = Math::abs(mouse_pos.x - p0.x);
		float e1 = Math::abs(mouse_pos.x - p2.x);
		float e2 = Math::abs(mouse_pos.y - p0.y);
		float e3 = Math::abs(mouse_pos.y - p2.y);
		if (mouse_pos.y < p0.y || mouse_pos.y > p2.y)
			e0 = e1 = Consts::INF;
		if (mouse_pos.x < p0.x || mouse_pos.x > p2.x)
			e2 = e3 = Consts::INF;
		if (e0 < edge_thickness || e1 < edge_thickness || e2 < edge_thickness
			|| e3 < edge_thickness)
		{
			if (e0 <= e1 && e0 <= e2 && e0 <= e3)
			{
				mouse_grab_type = GRAB_TYPE::EDGE_LEFT;
				return 1;
			}
			else if (e1 <= e0 && e1 <= e2 && e1 <= e3)
			{
				mouse_grab_type = GRAB_TYPE::EDGE_RIGHT;
				return 1;
			}
			else if (e2 <= e0 && e2 <= e1 && e2 <= e3)
			{
				mouse_grab_type = GRAB_TYPE::EDGE_TOP;
				return 1;
			}
			else
			{
				mouse_grab_type = GRAB_TYPE::EDGE_BOTTOM;
				return 1;
			}
		}
		// plane
		else if (mouse_pos.x >= p0.x && mouse_pos.x <= p2.x && mouse_pos.y >= p0.y
				 && mouse_pos.y <= p2.y)
		{
			mouse_grab_type = GRAB_TYPE::PLANE;
			return 1;
		}
	}

	return false;
}

void UIDesigner::cut_selected()
{
	copy_to_clipboard();
	destroy_selected();
}

void UIDesigner::copy_to_clipboard()
{
	if (!selected_elements.size())
		return;
	if (selected_elements.size() == 1 && canvas && selected_elements[0] == canvas->getPtr())
		return;

	// clear clipboard
	for (int i = 0; i < clipboard.size(); i++)
		clipboard[i].deleteLater();
	clipboard.clear();

	// copy
	for (int i = 0; i < selected_roots.size(); i++)
	{
		NodePtr n = selected_roots[i]->getNode()->clone();
		n->setEnabled(false);
		editor_logic.setEnabled(false);
		n->setParent(NodePtr());
		editor_logic.setEnabled(true);
		n->setSaveToWorldEnabledRecursive(false);
		n->setShowInEditorEnabledRecursive(false);
		clipboard.append(n);
	}
}

void UIDesigner::paste_from_clipboard()
{
	if (clipboard.empty() || !canvas)
		return;

	UI::ElementPtr parent;
	if (selected_elements.size() == 1)
		parent = selected_elements[0];

	VectorStack<UI::ElementPtr> s;
	for (int i = 0; i < clipboard.size(); i++)
	{
		NodePtr n = clipboard[i]->clone();
		n->setSaveToWorldEnabledRecursive(true);
		n->setShowInEditorEnabledRecursive(true);
		editor_logic.setEnabled(false);
		n->setParent(parent ? parent->getNode() : canvas->getNode());
		editor_logic.setEnabled(true);
		n->setEnabled(true);
		s.append(ComponentSystem::get()->getComponent<UI::Element>(n)->getPtr());
	}
	undo_manager->push(new CreatedWidgetCommand(selected_elements, s));
	select_elements(s);
}

void UIDesigner::duplicate_selected()
{
	// checks
	if (!selected_elements.size())
		return;
	if (selected_elements.size() == 1 && canvas && selected_elements[0] == canvas->getPtr())
		return;

	// duplicate
	VectorStack<UI::ElementPtr> s;
	for (int i = 0; i < selected_roots.size(); i++)
	{
		NodePtr cloned_node = selected_roots[i]->getNode()->clone();
		UI::Element *e = ComponentSystem::get()->getComponent<UI::Element>(cloned_node);
		s.append(e->getPtr());
	}

	// undo/redo
	undo_manager->push(new CreatedWidgetCommand(selected_elements, s));

	// select new elements
	select_elements(s);
}

void UIDesigner::destroy_selected()
{
	if (!selected_elements.size())
		return;

#ifdef EDITOR_PLUGIN
	if (sync_selection)
	{
		editor_selected_nodes.clear();
		UnigineEditor::Selection::setSelector(
			UnigineEditor::SelectorNodes::createObjectsSelector({}));
	}
#endif
	undo_manager->apply(new DeleteWidgetCommand(selected_roots));
}

void UIDesigner::select_all()
{
	if (!canvas)
		return;

	Vector<UI::ElementPtr> new_selection;
	std::function<void(const UI::ElementPtr &)> select_all_recursive;
	select_all_recursive = [&](const UI::ElementPtr &parent) {
		for (int i = 0; i < parent->getNumChildren(); i++)
		{
			UI::ElementPtr c = parent->getChild(i)->getPtr();
#ifndef EDITOR_PLUGIN
			if (c == trash_root)
				continue;
#endif

			new_selection.append(c);
			select_all_recursive(c);
		}
	};
	select_all_recursive(canvas->getPtr());

	if (selected_elements.size() != new_selection.size())
	{
		// select all
		undo_manager->apply(new SelectWidgetCommand(selected_elements, new_selection));
	}
	else
	{
		// deselect all
		undo_manager->apply(new SelectWidgetCommand(selected_elements, UI::ElementPtr()));
	}
}

void UIDesigner::deselect_all()
{
	undo_manager->apply(new SelectWidgetCommand(selected_elements, UI::ElementPtr()));
}

void UIDesigner::move_element_by_keyboard()
{
	if (!selected_elements.size())
		return;

	float offset_x = 0;
	float offset_y = 0;
	if (Input::isKeyDown(Input::KEY_LEFT))
		offset_x--;
	if (Input::isKeyDown(Input::KEY_RIGHT))
		offset_x++;
	if (Input::isKeyDown(Input::KEY_UP))
		offset_y--;
	if (Input::isKeyDown(Input::KEY_DOWN))
		offset_y++;
	if (Input::isKeyPressed(Input::KEY_ANY_CTRL))
	{
		offset_x *= grid_multiplier;
		offset_y *= grid_multiplier;
	}

	if (offset_x != 0 || offset_y != 0)
	{
		auto changed_state = new ChangedWidgetCommand(selected_roots);

		for (int i = 0; i < selected_roots.size(); i++)
		{
			UI::ElementPtr e = selected_roots[i];
			float px = e->getPositionX();
			float py = e->getPositionY();
			e->setPosition(px + offset_x, py + offset_y);
			if (!e->isFixedWidth())
				e->setRightOffset(e->getRightOffset() - offset_x);
			if (!e->isFixedHeight())
				e->setBottomOffset(e->getBottomOffset() - offset_y);
		}

		changed_state->saveState();
		undo_manager->push(changed_state);
	}
}

void UIDesigner::update_canvas()
{
	draw_manipulator();
	draw_selected_element_pivot();
	draw_selected_element_anchor();
	draw_rect_selection();
}

void UIDesigner::draw_manipulator()
{
	canvas_widget->clearLinePoints(0);	  // edges
	canvas_widget->clearLinePoints(1);	  // vertices
	canvas_widget->clearLinePoints(2);
	canvas_widget->clearLinePoints(3);
	canvas_widget->clearLinePoints(4);

	if (!selected_elements.size() || !view_manipulator)
		return;

	// rectangle (edges)
	canvas_widget->addLinePoint(0, vec3(manipulator.p0));
	canvas_widget->addLinePoint(0, vec3(manipulator.p1));
	canvas_widget->addLinePoint(0, vec3(manipulator.p2));
	canvas_widget->addLinePoint(0, vec3(manipulator.p3));
	canvas_widget->addLinePoint(0, vec3(manipulator.p0));
	canvas_widget->setLineColor(0, vec4(1, 1, 1, 1));

	draw_circle(1, vec3(manipulator.p0), vertex_radius, vec4(1, 1, 0, 1));
	draw_circle(2, vec3(manipulator.p1), vertex_radius, vec4(1, 1, 0, 1));
	draw_circle(3, vec3(manipulator.p2), vertex_radius, vec4(1, 1, 0, 1));
	draw_circle(4, vec3(manipulator.p3), vertex_radius, vec4(1, 1, 0, 1));
}

void UIDesigner::draw_circle(int id, const vec3 &point, float radius, const vec4 &color)
{
	int count = 8;
	float step = 1.0f / count;

	for (int i = 0; i <= count; i++)
	{
		float x = Math::sin(i * step * 2.0f * Consts::PI) * radius;
		float y = Math::cos(i * step * 2.0f * Consts::PI) * radius;
		canvas_widget->addLinePoint(id, point + vec3(x, y, 0));
	}

	canvas_widget->setLineColor(id, color);
}

void UIDesigner::draw_rect_selection()
{
	// rectange selection with background
	canvas_widget->clearPolygonPoints(5);
	canvas_widget->clearLinePoints(6);
	if (mouse_hold && is_selection_mode() && mouse_grab_type == GRAB_TYPE::NONE)
	{
		canvas_widget->addPolygonPoint(5, vec3(itof(mouse_down_pos.x), itof(mouse_down_pos.y), 0));
		canvas_widget->addPolygonPoint(5, vec3(itof(mouse_hold_pos.x), itof(mouse_down_pos.y), 0));
		canvas_widget->addPolygonPoint(5, vec3(itof(mouse_hold_pos.x), itof(mouse_hold_pos.y), 0));
		canvas_widget->addPolygonPoint(5, vec3(itof(mouse_down_pos.x), itof(mouse_hold_pos.y), 0));
		canvas_widget->addPolygonPoint(5, vec3(itof(mouse_down_pos.x), itof(mouse_down_pos.y), 0));
		canvas_widget->setPolygonColor(5, vec4(0, 0.74f, 1, 0.25f));

		canvas_widget->addLinePoint(6, vec3(itof(mouse_down_pos.x), itof(mouse_down_pos.y), 0));
		canvas_widget->addLinePoint(6, vec3(itof(mouse_hold_pos.x), itof(mouse_down_pos.y), 0));
		canvas_widget->addLinePoint(6, vec3(itof(mouse_hold_pos.x), itof(mouse_hold_pos.y), 0));
		canvas_widget->addLinePoint(6, vec3(itof(mouse_down_pos.x), itof(mouse_hold_pos.y), 0));
		canvas_widget->addLinePoint(6, vec3(itof(mouse_down_pos.x), itof(mouse_down_pos.y), 0));
		canvas_widget->setLineColor(6, vec4(0, 0.74f, 1, 1));
	}
}

void UIDesigner::draw_selected_element_pivot()
{
	canvas_widget->clearLinePoints(7);	  // pivot

	if (selected_elements.size() != 1 || !view_pivot)
		return;

	float pivot_x = selected_elements[0]->getPivotX();
	float pivot_y = selected_elements[0]->getPivotY();

	vec2 min_s = norm_to_window(selected_elements[0]->getNormalizedBoundMin());
	vec2 max_s = norm_to_window(selected_elements[0]->getNormalizedBoundMax());
	float screen_pivot_x = lerp(min_s.x, max_s.x, pivot_x);
	float screen_pivot_y = lerp(min_s.y, max_s.y, pivot_y);

	draw_circle(7, vec3(screen_pivot_x, screen_pivot_y, 0), vertex_radius * 0.5f,
		vec4(1.0f, 0.3f, 0.3f, 1));
}

void UIDesigner::draw_selected_element_anchor()
{
	canvas_widget->clearLinePoints(8);	   // selected element: parent's anchor rectangle
	canvas_widget->clearLinePoints(9);	   // selected element: anchor rectangle
	canvas_widget->clearLinePoints(10);	   // anchor LT
	canvas_widget->clearLinePoints(11);	   // anchor LB
	canvas_widget->clearLinePoints(12);	   // anchor RB
	canvas_widget->clearLinePoints(13);	   // anchor RT

	if (selected_elements.size() != 1 || !view_anchor)
		return;

	vec4 anchor = selected_elements[0]->getAnchor();
	vec2 min_n = vec2_zero;
	vec2 max_n = vec2_one;
	UI::Element *parent = selected_elements[0]->getParent();
	if (parent)
	{
		float parent_width_n =
			parent->getNormalizedBoundMax().x - parent->getNormalizedBoundMin().x;
		float parent_height_n =
			parent->getNormalizedBoundMax().y - parent->getNormalizedBoundMin().y;

		min_n.x = parent->getNormalizedBoundMin().x + anchor.x * parent_width_n;
		min_n.y = parent->getNormalizedBoundMin().y + anchor.y * parent_height_n;
		max_n.x = parent->getNormalizedBoundMin().x + anchor.z * parent_width_n;
		max_n.y = parent->getNormalizedBoundMin().y + anchor.w * parent_height_n;
	}
	else
	{
		min_n.x = anchor.x;
		min_n.y = anchor.y;
		max_n.x = anchor.z;
		max_n.y = anchor.w;
	}

	min_n = norm_to_window(min_n);
	max_n = norm_to_window(max_n);

	// parent's anchor rectangle
	if (parent)
	{
		vec4 parent_anchor = parent->anchor;
		vec2 p_min_n = vec2_zero;
		vec2 p_max_n = vec2_one;
		UI::Element *parent_parent = selected_elements[0]->getParent()->getParent();
		if (parent_parent)
		{
			float parent_width_n =
				parent_parent->getNormalizedBoundMax().x - parent_parent->getNormalizedBoundMin().x;
			float parent_height_n =
				parent_parent->getNormalizedBoundMax().y - parent_parent->getNormalizedBoundMin().y;
			p_min_n.x = parent_parent->getNormalizedBoundMin().x + parent_anchor.x * parent_width_n;
			p_min_n.y =
				parent_parent->getNormalizedBoundMin().y + parent_anchor.y * parent_height_n;
			p_max_n.x = parent_parent->getNormalizedBoundMin().x + parent_anchor.z * parent_width_n;
			p_max_n.y =
				parent_parent->getNormalizedBoundMin().y + parent_anchor.w * parent_height_n;
		}
		else
		{
			p_min_n.x = parent_anchor.x;
			p_min_n.y = parent_anchor.y;
			p_max_n.x = parent_anchor.z;
			p_max_n.y = parent_anchor.w;
		}

		p_min_n = norm_to_window(p_min_n);
		p_max_n = norm_to_window(p_max_n);

		canvas_widget->addLinePoint(8, vec3(p_min_n.x, p_min_n.y, 0));
		canvas_widget->addLinePoint(8, vec3(p_min_n.x, p_max_n.y, 0));
		canvas_widget->addLinePoint(8, vec3(p_max_n.x, p_max_n.y, 0));
		canvas_widget->addLinePoint(8, vec3(p_max_n.x, p_min_n.y, 0));
		canvas_widget->addLinePoint(8, vec3(p_min_n.x, p_min_n.y, 0));
		canvas_widget->setLineColor(8, vec4(1, 1, 1, 0.25f));
	}

	// anchor rectangle
	canvas_widget->addLinePoint(9, vec3(min_n.x, min_n.y, 0));
	canvas_widget->addLinePoint(9, vec3(min_n.x, max_n.y, 0));
	canvas_widget->addLinePoint(9, vec3(max_n.x, max_n.y, 0));
	canvas_widget->addLinePoint(9, vec3(max_n.x, min_n.y, 0));
	canvas_widget->addLinePoint(9, vec3(min_n.x, min_n.y, 0));
	canvas_widget->setLineColor(9, vec4(1, 1, 1, 0.5f));

	// left top anchor
	canvas_widget->addLinePoint(10, vec3(min_n.x, min_n.y, 0));
	canvas_widget->addLinePoint(10, vec3(min_n.x - anchor_size * 0.5f, min_n.y - anchor_size, 0));
	canvas_widget->addLinePoint(10, vec3(min_n.x - anchor_size, min_n.y - anchor_size * 0.5f, 0));
	canvas_widget->addLinePoint(10, vec3(min_n.x, min_n.y, 0));
	canvas_widget->setLineColor(10, vec4(1, 1, 1, 0.75f));

	// left bottom anchor
	canvas_widget->addLinePoint(11, vec3(min_n.x, max_n.y, 0));
	canvas_widget->addLinePoint(11, vec3(min_n.x - anchor_size, max_n.y + anchor_size * 0.5f, 0));
	canvas_widget->addLinePoint(11, vec3(min_n.x - anchor_size * 0.5f, max_n.y + anchor_size, 0));
	canvas_widget->addLinePoint(11, vec3(min_n.x, max_n.y, 0));
	canvas_widget->setLineColor(11, vec4(1, 1, 1, 0.75f));

	// right bottom anchor
	canvas_widget->addLinePoint(12, vec3(max_n.x, max_n.y, 0));
	canvas_widget->addLinePoint(12, vec3(max_n.x + anchor_size * 0.5f, max_n.y + anchor_size, 0));
	canvas_widget->addLinePoint(12, vec3(max_n.x + anchor_size, max_n.y + anchor_size * 0.5f, 0));
	canvas_widget->addLinePoint(12, vec3(max_n.x, max_n.y, 0));
	canvas_widget->setLineColor(12, vec4(1, 1, 1, 0.75f));

	// right top anchor
	canvas_widget->addLinePoint(13, vec3(max_n.x, min_n.y, 0));
	canvas_widget->addLinePoint(13, vec3(max_n.x + anchor_size, min_n.y - anchor_size * 0.5f, 0));
	canvas_widget->addLinePoint(13, vec3(max_n.x + anchor_size * 0.5f, min_n.y - anchor_size, 0));
	canvas_widget->addLinePoint(13, vec3(max_n.x, min_n.y, 0));
	canvas_widget->setLineColor(13, vec4(1, 1, 1, 0.75f));
}

void UIDesigner::draw_align_line_horizontal(float y)
{
	y = canvas_to_window(vec2(0, y)).y;
	canvas_widget->clearLinePoints(14);
	canvas_widget->addLinePoint(14, vec3(0, y, 0));
	canvas_widget->addLinePoint(14, vec3(itof(window_size.x), y, 0));
	canvas_widget->setLineColor(14, vec4(1, 0, 1, 1));
}

void UIDesigner::draw_align_line_vertical(float x)
{
	x = canvas_to_window(vec2(x, 0)).x;
	canvas_widget->clearLinePoints(15);
	canvas_widget->addLinePoint(15, vec3(x, 0, 0));
	canvas_widget->addLinePoint(15, vec3(x, itof(window_size.y), 0));
	canvas_widget->setLineColor(15, vec4(1, 0, 1, 1));
}

void UIDesigner::clear_selection()
{
	selected_elements.clear();
	canvas_widget->clearLinePoints(0);	  // edges
	canvas_widget->clearLinePoints(1);	  // vertices
	canvas_widget->clearLinePoints(2);
	canvas_widget->clearLinePoints(3);
	canvas_widget->clearLinePoints(4);
	// refresh hierarchy

#ifdef EDITOR_PLUGIN
	if (sync_selection)
	{
		UnigineEditor::Selection::setSelector(
			UnigineEditor::SelectorNodes::createObjectsSelector({}));
	}
#endif
}

void UIDesigner::find_elements_depth(const UI::ElementPtr &parent, float px_x, float px_y,
	Unigine::Vector<ElementDepth> &out_result, int depth) const
{
	for (int i = 0; i < parent->getNumChildren(); i++)
	{
		UI::ElementPtr c = parent->getChild(i)->getPtr();
		if (!c->isEnabled())
			continue;

		find_elements_depth(c, px_x, px_y, out_result, depth + 1);

		if (c->isHover(ftoi(px_x), ftoi(px_y)))
			out_result.append({c, depth});
	}
}

UI::ElementPtr UIDesigner::find_element(float px_x, float px_y) const
{
	if (!canvas)
		return UI::ElementPtr();

	return find_element(canvas->getPtr(), px_x, px_y);
}

UI::ElementPtr UIDesigner::find_element(const UI::ElementPtr &parent, float px_x, float px_y) const
{
	VectorStack<ElementDepth> all_elements;
	find_elements_depth(parent, px_x, px_y, all_elements);

	if (all_elements.size() == 0)
		return UI::ElementPtr();

	quickSort(
		all_elements.begin(), all_elements.end(), [](const ElementDepth &a, const ElementDepth &b) {
			int order_a = a.element->getOrder();
			int order_b = b.element->getOrder();
			if (order_a == order_b)
			{
				// prefer to select first more smaller elements
				vec4 ap = a.element->getWorldPosition();
				vec4 bp = b.element->getWorldPosition();
				float a_area = (ap.z - ap.x) * (ap.w - ap.y);
				float b_area = (bp.z - bp.x) * (bp.w - bp.y);
				return a_area < b_area;
			}
			return order_a > order_b;

			/*
			if (a.depth == b.depth)
			{
				// prefer to select first more smaller elements
				vec4 ap = a.element->getWorldPosition();
				vec4 bp = b.element->getWorldPosition();
				float a_area = (ap.z - ap.x) * (ap.w - ap.y);
				float b_area = (bp.z - bp.x) * (bp.w - bp.y);
				return a_area < b_area;
			}
			return a.depth > b.depth;
			*/
		});

	return all_elements[0].element;
}

UI::ElementPtr UIDesigner::find_next_element(
	float px_x, float px_y, const UI::ElementPtr &prev_element) const
{
	if (!canvas)
		return UI::ElementPtr();
	return find_next_element(canvas->getPtr(), px_x, px_y, prev_element);
}

UI::ElementPtr UIDesigner::find_next_element(
	const UI::ElementPtr &parent, float px_x, float px_y, const UI::ElementPtr &prev_element) const
{
	VectorStack<ElementDepth> all_elements;
	find_elements_depth(parent, px_x, px_y, all_elements);

	if (all_elements.size() == 0)
		return UI::ElementPtr();

	quickSort(
		all_elements.begin(), all_elements.end(), [](const ElementDepth &a, const ElementDepth &b) {
			int order_a = a.element->getOrder();
			int order_b = b.element->getOrder();
			if (order_a == order_b)
			{
				// prefer to select first more smaller elements
				vec4 ap = a.element->getWorldPosition();
				vec4 bp = b.element->getWorldPosition();
				float a_area = (ap.z - ap.x) * (ap.w - ap.y);
				float b_area = (bp.z - bp.x) * (bp.w - bp.y);
				return a_area < b_area;
			}
			return order_a > order_b;

			/*
			if (a.depth == b.depth)
			{
				// prefer to select first more smaller elements
				vec4 ap = a.element->getWorldPosition();
				vec4 bp = b.element->getWorldPosition();
				float a_area = (ap.z - ap.x) * (ap.w - ap.y);
				float b_area = (bp.z - bp.x) * (bp.w - bp.y);
				return a_area < b_area;
			}
			return a.depth > b.depth;
			*/
		});

	for (int i = 0; i < all_elements.size(); i++)
		if (all_elements[i].element == prev_element)
		{
			if (i + 1 < all_elements.size())
				return all_elements[i + 1].element;
			else
				return all_elements[0].element;
		}

	return UI::ElementPtr();
}

Vector<UI::ElementPtr> UIDesigner::find_elements(const vec4 &rect) const
{
	Vector<UI::ElementPtr> result;
	if (canvas)
		find_elements(canvas->getPtr(), rect, result);
	return result;
}

void UIDesigner::find_elements(const UI::ElementPtr &parent, const vec4 &rect1,
	Unigine::Vector<UI::ElementPtr> &out_result) const
{
	for (int i = 0; i < parent->getNumChildren(); i++)
	{
		UI::ElementPtr c = parent->getChild(i)->getPtr();
		if (!c->isEnabled())
			continue;

		find_elements(c, rect1, out_result);

		if (c->isHover(ftoi(rect1.x), ftoi(rect1.y), ftoi(rect1.z), ftoi(rect1.w)))
			out_result.append(c);
	}
}

void UIDesigner::refresh_selected_element_changes()
{
	delete selected_element_changes;
	if (selected_elements.size())
		selected_element_changes = new ChangedWidgetCommand(selected_elements);
	else
		selected_element_changes = nullptr;
	widget_moved = false;
	parameter_changed = false;
	parameter_released = false;
}

void UIDesigner::find_all_snappable_elements()
{
	std::function<void(UI::Element *)> func_recursive;
	func_recursive = [&](UI::Element *parent) {
		for (int i = 0; i < parent->getNumChildren(); i++)
		{
			UI::Element *c = parent->getChild(i);
#ifndef EDITOR_PLUGIN
			if (c == trash_root)
				continue;
#endif
			if (!c->isEnabled())
				continue;
			if (selected_roots.contains(c))
				continue;

			if (dynamic_cast<UI::ElementWidget *>(c))
				all_snappable_elements.append(c);

			func_recursive(c);
		}
	};

	all_snappable_elements.clear();
	if (canvas)
		func_recursive(canvas.get());
}

float UIDesigner::snap_x(float value)
{
	if (use_snap_to_grid)
		value = itof(ftoi(value / grid) * grid);

	if (use_snap_to_elements)
	{
		for (int i = 0; i < all_snappable_elements.size(); i++)
		{
			UI::Element *c = all_snappable_elements[i];
			vec4 pos = c->getWorldPosition();

			if (Math::abs(value - pos.x) < 5)
			{
				value = pos.x;	  // align left border to left border
				draw_align_line_vertical(pos.x);
				break;
			}

			if (Math::abs(value - pos.z) < 5)
			{
				value = pos.z;	  // align left border to right border
				draw_align_line_vertical(pos.z);
				break;
			}
		}
	}

	return value;
}

float UIDesigner::snap_y(float value)
{
	if (use_snap_to_grid)
		value = itof(ftoi(value / grid) * grid);

	if (use_snap_to_elements)
	{
		for (int i = 0; i < all_snappable_elements.size(); i++)
		{
			UI::Element *c = all_snappable_elements[i];
			vec4 pos = c->getWorldPosition();

			if (Math::abs(value - pos.y) < 5)
			{
				value = pos.y;	  // align top border to top border
				draw_align_line_horizontal(pos.y);
				break;
			}

			if (Math::abs(value - pos.w) < 5)
			{
				value = pos.w;	  // align top border to bottom border
				draw_align_line_horizontal(pos.w);
				break;
			}
		}
	}

	return value;
}

vec2 UIDesigner::snap(const vec2 &value)
{
	return vec2(snap_x(value.x), snap_y(value.y));
}

vec2 UIDesigner::snap(float x, float y, float w, float h)
{
	// snap to grid
	if (use_snap_to_grid)
	{
		x = itof(ftoi(x / grid) * grid);
		y = itof(ftoi(y / grid) * grid);
	}

	// snap to elements
	if (!use_snap_to_elements)
		return vec2(x, y);

	for (int i = 0; i < all_snappable_elements.size(); i++)
	{
		UI::Element *c = all_snappable_elements[i];
		vec4 pos = c->getWorldPosition();

		if (Math::abs(x - pos.x) < 5)
		{
			x = pos.x;	  // align left border to left border
			draw_align_line_vertical(pos.x);
			break;
		}

		if (Math::abs(x - pos.z) < 5)
		{
			x = pos.z;	  // align left border to right border
			draw_align_line_vertical(pos.z);
			break;
		}

		if (Math::abs(x + w - pos.x) < 5)
		{
			x = pos.x - w;	  // align right border to left border
			draw_align_line_vertical(pos.x);
			break;
		}

		if (Math::abs(x + w - pos.z) < 5)
		{
			x = pos.z - w;	  // align right border to right border
			draw_align_line_vertical(pos.z);
			break;
		}
	}

	for (int i = 0; i < all_snappable_elements.size(); i++)
	{
		UI::Element *c = all_snappable_elements[i];
		vec4 pos = c->getWorldPosition();

		if (Math::abs(y - pos.y) < 5)
		{
			y = pos.y;	  // align top border to top border
			draw_align_line_horizontal(pos.y);
			break;
		}

		if (Math::abs(y - pos.w) < 5)
		{
			y = pos.w;	  // align top border to bottom border
			draw_align_line_horizontal(pos.w);
			break;
		}

		if (Math::abs(y + h - pos.y) < 5)
		{
			y = pos.y - h;	  // align bottom border to top border
			draw_align_line_horizontal(pos.y);
			break;
		}

		if (Math::abs(y + h - pos.w) < 5)
		{
			y = pos.w - h;	  // align bottom border to bottom border
			draw_align_line_horizontal(pos.w);
			break;
		}
	}

	return vec2(x, y);
}

void UIDesigner::show_file_dialog(const char *title, const char *filter)
{
	file_dialog->setText(title);
	file_dialog->setFilter(filter);
	file_dialog->getCancelButton()->getEventClicked().connect(
		file_dialog_cancel, [this]() { close_file_dialog(); });
	Gui::getCurrent()->addChild(file_dialog, Gui::ALIGN_OVERLAP | Gui::ALIGN_CENTER);
	file_dialog_show = true;
}

void UIDesigner::close_file_dialog()
{
	file_dialog_show = false;
	file_dialog_ok.disconnect();
	file_dialog_cancel.disconnect();
	Gui::getCurrent()->removeChild(file_dialog);
}

int UIDesigner::get_position_x(const UI::ElementPtr &element) const
{
	return ftoi(element->getPositionX() * element->getCanvas()->getScreenWidth());
}

int UIDesigner::get_position_y(const UI::ElementPtr &element) const
{
	return ftoi(element->getPositionY() * element->getCanvas()->getScreenHeight());
}

int UIDesigner::get_width(const UI::ElementPtr &element) const
{
	return ftoi((element->getNormalizedBoundMax().x - element->getNormalizedBoundMin().x)
				* element->getCanvas()->getScreenWidth());
}

int UIDesigner::get_height(const UI::ElementPtr &element) const
{
	return ftoi((element->getNormalizedBoundMax().y - element->getNormalizedBoundMin().y)
				* element->getCanvas()->getScreenHeight());
}

int UIDesigner::get_screen_position_x(const UI::ElementPtr &element) const
{
	return ftoi(element->getNormalizedBoundMin().x * element->getCanvas()->getScreenWidth());
}

int UIDesigner::get_screen_position_y(const UI::ElementPtr &element) const
{
	return ftoi(element->getNormalizedBoundMin().y * element->getCanvas()->getScreenHeight());
}

int UIDesigner::get_screen_position_x1(const UI::ElementPtr &element) const
{
	return ftoi(element->getNormalizedBoundMax().x * element->getCanvas()->getScreenWidth());
}

int UIDesigner::get_screen_position_y1(const UI::ElementPtr &element) const
{
	return ftoi(element->getNormalizedBoundMax().y * element->getCanvas()->getScreenHeight());
}

void UIDesigner::DesignerEditorLogic::nodeReparented(const NodePtr &node)
{
	if (!isEnabled())
		return;

	Vector<UI::Element *> elements;
	ComponentSystem::get()->getComponentsInChildren<UI::Element>(node, elements);
	for (int i = 0; i < elements.size(); i++)
		elements[i]->applyNodeHierarchyChanges();
}

void UIDesigner::DesignerEditorLogic::nodeReordered(const NodePtr &node)
{
	if (!isEnabled())
		return;
}

void UIDesigner::DesignerEditorLogic::propertyChanged(const UGUID &guid)
{
	if (!isEnabled())
		return;

	// TODO: need to understand, why propertyChanged() calls every frame!
	// Looks like a bug

	/*
	bool selected = false;
	auto designer = UIDesigner::get();
	for (int i = 0; i < designer->selected_elements.size(); i++)
	{
		if (designer->selected_elements[i]->getProperty()->getGUID() == guid)
		{
			selected = true;
			break;
		}
	}
	if (!selected)
		return;

	designer->refresh_selected_element_changes();
	Log::message("REFRESHED!\n");
	*/
}
