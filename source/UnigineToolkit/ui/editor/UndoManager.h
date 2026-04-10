#pragma once
#include <UnigineVector.h>

class IUndoCommand
{
public:
	virtual ~IUndoCommand() = default;

	virtual void apply() = 0;
	virtual void undo() = 0;
};

class UndoGroupCommand : public IUndoCommand
{
public:
	~UndoGroupCommand();

	void push(IUndoCommand *command);
	void apply() override;
	void undo() override;

private:
	Unigine::Vector<IUndoCommand *> commands;
};

class UndoManager
{
public:
	UndoManager(int capacity = 30);
	~UndoManager();

	void apply(IUndoCommand *command);	  // push and call ICommand::apply()
	void push(IUndoCommand *command);	  // push to queue only

	void undo();
	void redo();
	void clear();

private:
	int index, max_index;
	int capacity;	 // max undo buffer
	Unigine::Vector<IUndoCommand *> undo_vector;
};