#pragma once

#include <UnigineComponentSystem.h>
#include <UnigineString.h>
#include <UnigineVector.h>
#include <UnigineWidgets.h>

class DialogueInteractable;

struct DialogueLine
{
	Unigine::String text;
	float duration = 0.0f; // 0 = wait until interact / explicit close
};

class DialogueController: public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(DialogueController, Unigine::ComponentBase)
	COMPONENT_INIT(init)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	PROP_PARAM(File, font)
	PROP_PARAM(Int, fontSize, 30)
	PROP_PARAM(Color, fontColor, Unigine::Math::vec4(0.92f, 0.9f, 0.84f, 1.0f))
	PROP_PARAM(Int, maxTextWidth, 1100)
	PROP_PARAM(Int, textHeight, 180)
	PROP_PARAM(Int, bottomOffset, 96)

	static DialogueController *get();

	bool beginDialogue(DialogueInteractable *source, const Unigine::Vector<DialogueLine> &lines,
		const char *music_event, const char *music_layer, bool restore_music_on_end);
	void advance(DialogueInteractable *source);
	void close(DialogueInteractable *source);

	bool isActive() const noexcept { return _active_source != nullptr; }
	bool isActiveSource(const DialogueInteractable *source) const noexcept { return _active_source == source; }
	bool canStart(const DialogueInteractable *source) const noexcept;

private:
	void init();
	void update();
	void shutdown();

	void showCurrentLine();
	void finish();
	void ensureLayout();
	void hide();

private:
	Unigine::GuiPtr _gui;
	Unigine::WidgetLabelPtr _label;

	DialogueInteractable *_active_source = nullptr;
	Unigine::Vector<DialogueLine> _lines;
	int _line_index = 0;
	float _line_timer = 0.0f;
	Unigine::String _music_layer;
	bool _restore_music_on_end = false;
	bool _music_started = false;
};
