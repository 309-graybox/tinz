#include "UndoManager.h"

#ifdef EDITOR_PLUGIN
	#include <editor/UnigineUndo.h>
#endif

using namespace Unigine;
using namespace Math;

#ifdef EDITOR_PLUGIN
class ActionWrapper : public UnigineEditor::Action
{
public:
	ActionWrapper(IUndoCommand *in_command) { command = in_command; }
	~ActionWrapper() { delete command; }

	void apply() override { command->apply(); }
	void undo() override { command->undo(); }
	void redo() override { command->apply(); }

private:
	IUndoCommand *command;
};
#endif

UndoGroupCommand::~UndoGroupCommand()
{
	for (int i = 0; i < commands.size(); i++)
		delete commands[i];
	commands.clear();
}

void UndoGroupCommand::push(IUndoCommand *command)
{
	commands.append(command);
}

void UndoGroupCommand::apply()
{
	for (int i = 0; i < commands.size(); i++)
		commands[i]->apply();
}

void UndoGroupCommand::undo()
{
	for (int i = 0; i < commands.size(); i++)
		commands[i]->undo();
}

UndoManager::UndoManager(int in_capacity)
{
	index = -1;
	max_index = -1;
	capacity = in_capacity;
	undo_vector.resize(nullptr, capacity);
}

UndoManager::~UndoManager()
{
	for (int i = 0; i < undo_vector.size(); i++)
		delete undo_vector[i];
	undo_vector.clear();
}

void UndoManager::apply(IUndoCommand *command)
{
#ifdef EDITOR_PLUGIN
	UnigineEditor::Undo::apply(new ActionWrapper(command));
#else
	if (!command)
		return;

	push(command);
	command->apply();
#endif
}

void UndoManager::push(IUndoCommand *command)
{
	if (!command)
		return;

#ifdef EDITOR_PLUGIN
	UnigineEditor::Undo::push(new ActionWrapper(command));
#else
	index++;

	// check of reaching max apply/push operations
	if (index >= capacity)
	{
		index = capacity - 1;
		delete undo_vector[0];
		for (int i = 0; i < undo_vector.size() - 1; i++)
			undo_vector[i] = undo_vector[i + 1];
	}
	// clear previous history after some undo calls
	else
	{
		for (int i = index; i <= max_index; i++)
		{
			delete undo_vector[i];
			undo_vector[i] = nullptr;
		}
	}

	undo_vector[index] = command;
	max_index = index;
#endif
}

void UndoManager::undo()
{
#ifdef EDITOR_PLUGIN
	UnigineEditor::Undo::undo();
#else
	if (index < 0)
		return;

	undo_vector[index]->undo();
	index--;
#endif
}

void UndoManager::redo()
{
#ifdef EDITOR_PLUGIN
	UnigineEditor::Undo::redo();
#else

	index++;
	if (index <= max_index)
		undo_vector[index]->apply();
	else
		index = max_index;
#endif
}

void UndoManager::clear()
{
	for (int i = 0; i < undo_vector.size(); i++)
	{
		delete undo_vector[i];
		undo_vector[i] = nullptr;
	}
	index = -1;
	max_index = -1;
}
