#include "SoundPlayer.h"

#include "../elements/Element.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(SoundPlayer);

void SoundPlayer::init()
{
	if (sound_hover_file.get())
		sound_hover = AmbientSource::create(sound_hover_file);
	if (sound_press_file.get())
		sound_press = AmbientSource::create(sound_press_file);
	if (sound_focus_file.get())
		sound_focus = AmbientSource::create(sound_focus_file);

	ElementFocusable *focusable = getComponent<ElementFocusable>(node);
	if (!focusable)
	{
		Log::error(
			"UI::SoundPlayer::init(): Attach this component to ElementFocusable element (button, "
			"slider, checkbox, etc.)! Node: \"%s\"\n",
			node->getName());
		return;
	}

	focusable->getEventStateChanged().connect(*this, [this](ElementFocusable *e) {
		UI::ElementFocusable::State state = e->getState();
		switch (state)
		{
		case UI::ElementFocusable::State::Hover:
			if (sound_hover)
			{
				sound_hover->setGain(volume);
				sound_hover->play();
			}
			break;
		case UI::ElementFocusable::State::Press:
			if (sound_press)
			{
				sound_press->setGain(volume);
				sound_press->play();
			}
			break;
		case UI::ElementFocusable::State::Focus:
			if (sound_focus)
			{
				sound_focus->setGain(volume);
				sound_focus->play();
			}
			break;
		default:
			break;
		}
	});
}

void SoundPlayer::shutdown()
{
	sound_hover.deleteLater();
	sound_press.deleteLater();
	sound_focus.deleteLater();
}
