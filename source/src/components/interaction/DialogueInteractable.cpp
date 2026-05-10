#include "components/interaction/DialogueInteractable.h"

#include <UnigineLog.h>

REGISTER_COMPONENT(DialogueInteractable)

using namespace Unigine;

float DialogueInteractable::getRangeExitDistance() const
{
	const float enter_distance = range;
	const float exit_distance = exitRange;
	return exit_distance > enter_distance ? exit_distance : enter_distance;
}

bool DialogueInteractable::canInteract(const NodePtr &interactor) const
{
	if (!Interactable::canInteract(interactor))
		return false;

	DialogueController *controller = DialogueController::get();
	if (controller && controller->isActiveSource(this))
		return advanceOnInteract;

	return getTriggerMode() == TriggerMode::Interact && canBeginDialogue();
}

void DialogueInteractable::onRangeEntered(const NodePtr &interactor)
{
	Interactable::onRangeEntered(interactor);

	if (getTriggerMode() == TriggerMode::Range)
		tryBeginDialogue();
}

void DialogueInteractable::onRangeLeft(const NodePtr &interactor)
{
	Interactable::onRangeLeft(interactor);

	if (!hideOnRangeLeft)
		return;

	if (DialogueController *controller = DialogueController::get())
		controller->close(this);
}

void DialogueInteractable::onInteract(const NodePtr &interactor)
{
	Interactable::onInteract(interactor);

	DialogueController *controller = DialogueController::get();
	if (controller && controller->isActiveSource(this))
	{
		if (advanceOnInteract)
			controller->advance(this);
		return;
	}

	if (getTriggerMode() == TriggerMode::Interact)
		tryBeginDialogue();
}

bool DialogueInteractable::tryBeginDialogue()
{
	if (!canBeginDialogue())
		return false;

	DialogueController *controller = DialogueController::get();
	if (!controller)
	{
		Log::warning("DialogueInteractable: no DialogueController in world\n");
		return false;
	}

	Vector<DialogueLine> runtime_lines = collectLines();
	const bool started = controller->beginDialogue(
		this, runtime_lines, musicEvent.get(), musicLayer.get(), (int)restoreMusicOnEnd != 0);
	if (started && playOnce)
		_consumed = true;
	return started;
}

bool DialogueInteractable::canBeginDialogue() const
{
	if (_consumed)
		return false;
	if (const_cast<DialogueInteractable *>(this)->lines.size() == 0)
		return false;

	DialogueController *controller = DialogueController::get();
	return !controller || controller->canStart(this);
}

Vector<DialogueLine> DialogueInteractable::collectLines()
{
	Vector<DialogueLine> out;
	for (int i = 0; i < lines.size(); ++i)
	{
		auto &cfg = lines[i];
		if (cfg->text.get()[0] == '\0')
			continue;

		DialogueLine line;
		line.text = cfg->text.get();
		line.duration = cfg->duration;
		out.append(line);
	}
	return out;
}
