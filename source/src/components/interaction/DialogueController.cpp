#include "components/interaction/DialogueController.h"

#include "components/interaction/DialogueInteractable.h"
#include "audio/SoundManager.h"

#include <UnigineGame.h>
#include <UnigineGui.h>
#include <UnigineLog.h>

REGISTER_COMPONENT(DialogueController)

using namespace Unigine;
using namespace Unigine::Math;

DialogueController *DialogueController::get()
{
	return ComponentSystem::get()->getComponentInWorld<DialogueController>();
}

void DialogueController::init()
{
	_gui = Gui::getCurrent();

	_label = WidgetLabel::create(_gui, "");
	_label->setFont(font);
	_label->setFontSize(fontSize);
	_label->setFontColor(fontColor);
	_label->setFontWrap(1);
	_gui->addChild(_label, Gui::ALIGN_BOTTOM);

	ensureLayout();
	hide();
}

void DialogueController::update()
{
	if (!_active_source)
		return;

	// While the game is paused (e.g. the pause menu is open) hide the line and
	// freeze the timer. Pause == game time stopped (Game::setScale(0)); the menu
	// also pauses dialogue audio globally via SoundManager::setPaused.
	const bool paused = Game::getScale() <= 0.0f;
	if (paused != _hidden_by_pause)
	{
		_hidden_by_pause = paused;
		if (paused)
			hide();
		else
			showCurrentLine();
	}
	if (paused)
		return;

	ensureLayout();

	const float duration = _lines[_line_index].duration;
	if (duration <= 0.0f)
		return;

	_line_timer += Game::getIFps();
	if (_line_timer >= duration)
		advance(_active_source);
}

void DialogueController::shutdown()
{
	if (_active_source)
		finish();

	if (_label)
	{
		_label.deleteLater();
		_label.clear();
	}
	_gui.clear();
}

bool DialogueController::beginDialogue(DialogueInteractable *source,
	const Vector<DialogueLine> &lines, const char *music_event, const char *music_layer,
	bool restore_music_on_end)
{
	if (!source || lines.empty())
		return false;
	if (!canStart(source))
		return false;

	_active_source = source;
	_lines = lines;
	_line_index = 0;
	_line_timer = 0.0f;
	_music_layer = (music_layer && *music_layer) ? music_layer : "dialogue";
	_restore_music_on_end = restore_music_on_end;
	_music_started = music_event && *music_event;

	if (_music_started)
	{
		if (_restore_music_on_end)
			audio::SoundManager::pushMusicLayer(_music_layer.get(), music_event);
		else
			audio::SoundManager::playMusicLayer(_music_layer.get(), music_event);
	}

	showCurrentLine();
	return true;
}

void DialogueController::advance(DialogueInteractable *source)
{
	if (!isActiveSource(source))
		return;

	++_line_index;
	_line_timer = 0.0f;

	if (_line_index >= _lines.size())
	{
		finish();
		return;
	}

	showCurrentLine();
}

void DialogueController::close(DialogueInteractable *source)
{
	if (!isActiveSource(source))
		return;

	finish();
}

bool DialogueController::canStart(const DialogueInteractable *source) const noexcept
{
	return !_active_source || _active_source == source;
}

void DialogueController::showCurrentLine()
{
	if (!_label || _line_index < 0 || _line_index >= _lines.size())
		return;

	_label->setText(_lines[_line_index].text.get());
	_label->setHidden(false);
}

void DialogueController::finish()
{
	if (_music_started && _restore_music_on_end)
		audio::SoundManager::popMusicLayer(_music_layer.get());

	_active_source = nullptr;
	_lines.clear();
	_line_index = 0;
	_line_timer = 0.0f;
	_music_layer.clear();
	_restore_music_on_end = false;
	_music_started = false;
	_hidden_by_pause = false;
	hide();
}

void DialogueController::ensureLayout()
{
	if (!_gui || !_label)
		return;

	const int gui_width = max(_gui->getWidth(), 1);
	const int width = min((int)maxTextWidth, max(gui_width - 120, 240));

	_label->setWidth(width);
	_label->setHeight(max((int)textHeight, (int)fontSize + 8));
	_label->setPosition(0, -(int)bottomOffset);
}

void DialogueController::hide()
{
	if (_label)
	{
		_label->setText("");
		_label->setHidden(true);
	}
}
