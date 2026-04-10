#pragma once
#include "../elements/Element.h"
#include "UndoManager.h"

#include <UnigineStreams.h>

#ifdef EDITOR_PLUGIN
	#include <editor/UnigineActions.h>
#endif

class CreatedWidgetCommand : public IUndoCommand
{
public:
	CreatedWidgetCommand(
		const Unigine::Vector<UI::ElementPtr> &prev_elements, const UI::ElementPtr &element);
	CreatedWidgetCommand(const Unigine::Vector<UI::ElementPtr> &prev_elements,
		const Unigine::Vector<UI::ElementPtr> &elements);
	~CreatedWidgetCommand();
	void apply() override;
	void undo() override;

private:
#ifdef EDITOR_PLUGIN
	UnigineEditor::CreateNodesAction *action;
#endif
	Unigine::Vector<Unigine::NodePtr> parents;
	Unigine::Vector<UI::ElementPtr> elements;
	Unigine::Vector<UI::ElementPtr> prev_elements;
};

class SelectWidgetCommand : public IUndoCommand
{
public:
	SelectWidgetCommand(
		const Unigine::Vector<UI::ElementPtr> &prev_elements, const UI::ElementPtr &element);
	SelectWidgetCommand(const Unigine::Vector<UI::ElementPtr> &prev_elements,
		const Unigine::Vector<UI::ElementPtr> &elements);
	void apply() override;
	void undo() override;

private:
	Unigine::Vector<UI::ElementPtr> prev_elements;
	Unigine::Vector<UI::ElementPtr> elements;
};

class ReparentWidgetCommand : public IUndoCommand
{
public:
	ReparentWidgetCommand(
		const Unigine::Vector<UI::ElementPtr> &elements, const UI::ElementPtr &parent_element);
	void apply() override;
	void undo() override;

private:
	Unigine::Vector<UI::ElementPtr> elements;
	Unigine::Vector<UI::ElementPtr> elements_parents;
	Unigine::Vector<int> elements_indices;
	UI::ElementPtr new_parent_element;
};

class ReorderWidgetCommand : public IUndoCommand
{
public:
	ReorderWidgetCommand(const Unigine::Vector<UI::ElementPtr> &elements,
		const UI::ElementPtr &ref_element, bool place_before);
	void apply() override;
	void undo() override;

private:
	Unigine::Vector<UI::ElementPtr> elements;
	Unigine::Vector<UI::ElementPtr> elements_parents;
	Unigine::Vector<int> elements_indices;
	UI::ElementPtr ref_element;
	bool place_before;
};

class ChangedWidgetCommand : public IUndoCommand
{
public:
	ChangedWidgetCommand(const UI::ElementPtr &element);
	ChangedWidgetCommand(const Unigine::Vector<UI::ElementPtr> &elements);
	void saveState();

	void apply() override;
	void undo() override;

private:
	Unigine::Vector<UI::ElementPtr> elements;
	Unigine::Vector<Unigine::BlobPtr> prev_states;
	Unigine::Vector<Unigine::BlobPtr> now_states;
};

class DeleteWidgetCommand : public IUndoCommand
{
public:
	DeleteWidgetCommand(const UI::ElementPtr &element);
	DeleteWidgetCommand(const Unigine::Vector<UI::ElementPtr> &elements);
	~DeleteWidgetCommand();
	void apply() override;
	void undo() override;

private:
#ifdef EDITOR_PLUGIN
	UnigineEditor::RemoveNodesAction *action;
#endif
	Unigine::Vector<Unigine::NodePtr> parents;
	Unigine::Vector<UI::ElementPtr> elements;
};
