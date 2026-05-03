#pragma once

#include "DialogueController.h"
#include "Interactable.h"

struct DialogueLineInfo: Unigine::ComponentStruct
{
	PROP_PARAM(String, text)
	PROP_PARAM(Float, duration, 0.0f, "Duration", "0 = wait until interact / close")
};

class DialogueInteractable: public Interactable
{
public:
	COMPONENT_DEFINE(DialogueInteractable, Interactable)

	enum class TriggerMode : int
	{
		Range = 0,
		Interact = 1,
	};

	PROP_PARAM(Float, exitRange, 4.0f, "Exit Range", "Distance where range dialogue closes")
	PROP_PARAM(Switch, triggerMode, 1, "Range,Interact", "Trigger Mode")
	PROP_ARRAY_STRUCT(DialogueLineInfo, lines)

	PROP_PARAM(String, musicEvent, "", "Music Event", "SoundManager event id or direct audio path")
	PROP_PARAM(String, musicLayer, "dialogue", "Music Layer",
		"Named SoundManager music layer; use 'music' to replace background")
	PROP_PARAM(Toggle, restoreMusicOnEnd, true, "Restore Music On End",
		"Restore previous music in this layer when dialogue ends; other layers keep playing")
	PROP_PARAM(Toggle, hideOnRangeLeft, true)
	PROP_PARAM(Toggle, playOnce, false)
	PROP_PARAM(Toggle, advanceOnInteract, true)

	float getRangeExitDistance() const override;
	bool canInteract(const Unigine::NodePtr &interactor) const override;

protected:
	void onRangeEntered(const Unigine::NodePtr &interactor) override;
	void onRangeLeft(const Unigine::NodePtr &interactor) override;
	void onInteract(const Unigine::NodePtr &interactor) override;

private:
	TriggerMode getTriggerMode() const noexcept { return static_cast<TriggerMode>(static_cast<int>(triggerMode)); }

	bool tryBeginDialogue();
	bool canBeginDialogue() const;
	Unigine::Vector<DialogueLine> collectLines();

	bool _consumed = false;
};
