#include "UndoCommands.h"

#include "UIDesigner.h"

#ifdef EDITOR_PLUGIN
	#include <editor/UnigineActions.h>
	#include <editor/UnigineAssetDialogs.h>
	#include <editor/UnigineConstants.h>
	#include <editor/UnigineEngineGuiWindow.h>
	#include <editor/UnigineSelection.h>
	#include <editor/UnigineSelector.h>
	#include <editor/UnigineWindowManager.h>
#endif

using namespace Unigine;
using namespace Math;

// --------------------------------------------------------------------------------------------------

CreatedWidgetCommand::CreatedWidgetCommand(
	const Unigine::Vector<UI::ElementPtr> &in_prev_elements, const UI::ElementPtr &in_element)
{
#ifdef EDITOR_PLUGIN
	action = new UnigineEditor::CreateNodesAction(in_element->getNode());
#endif

	parents.append(in_element->getNode()->getParent());
	elements.append(in_element);
	prev_elements = in_prev_elements;
}

CreatedWidgetCommand::CreatedWidgetCommand(const Unigine::Vector<UI::ElementPtr> &in_prev_elements,
	const Unigine::Vector<UI::ElementPtr> &in_elements)
{
#ifdef EDITOR_PLUGIN
	Vector<NodePtr> nodes;
	for (int i = 0; i < in_elements.size(); i++)
		nodes.append(in_elements[i]->getNode());
	action = new UnigineEditor::CreateNodesAction(nodes);
#endif

	elements = in_elements;
	for (int i = 0; i < elements.size(); i++)
		parents.append(elements[i]->getNode()->getParent());
	prev_elements = in_prev_elements;
}

CreatedWidgetCommand::~CreatedWidgetCommand()
{
#ifdef EDITOR_PLUGIN
	delete action;
	action = nullptr;
#else
	for (int i = 0; i < elements.size(); i++)
	{
		auto &e = elements[i];
		if (e && e->getNode()
			&& e->getNode()->getParent() == UIDesigner::get()->trash_root->getNode())
			e->getNode().deleteLater();
	}
#endif
}

void CreatedWidgetCommand::apply()
{
#ifdef EDITOR_PLUGIN
	action->redo();
#else
	for (int i = 0; i < elements.size(); i++)
	{
		NodePtr n = elements[i]->getNode();
		n->setParent(parents[i]);
		n->setSaveToWorldEnabledRecursive(true);
		n->setShowInEditorEnabledRecursive(true);
		elements[i]->applyNodeHierarchyChanges(false);
	}
#endif
	UIDesigner::get()->select_elements(elements, true);
}

void CreatedWidgetCommand::undo()
{
#ifdef EDITOR_PLUGIN
	UnigineEditor::Selection::setSelector(UnigineEditor::SelectorNodes::createObjectsSelector({}));
	action->undo();
#else
	for (int i = 0; i < elements.size(); i++)
	{
		NodePtr n = elements[i]->getNode();
		n->setParent(UIDesigner::get()->trash_root->getNode());
		n->setSaveToWorldEnabledRecursive(false);
		n->setShowInEditorEnabledRecursive(false);
		elements[i]->applyNodeHierarchyChanges(false);
	}
#endif
	UIDesigner::get()->select_elements(prev_elements, true);
}

// --------------------------------------------------------------------------------------------------

SelectWidgetCommand::SelectWidgetCommand(
	const Unigine::Vector<UI::ElementPtr> &in_prev_elements, const UI::ElementPtr &in_element)
{
	prev_elements = in_prev_elements;
	if (in_element)
		elements.append(in_element);
}

SelectWidgetCommand::SelectWidgetCommand(const Unigine::Vector<UI::ElementPtr> &in_prev_elements,
	const Unigine::Vector<UI::ElementPtr> &in_elements)
{
	prev_elements = in_prev_elements;
	elements = in_elements;
}

void SelectWidgetCommand::apply()
{
	UIDesigner::get()->select_elements(elements, true);
}

void SelectWidgetCommand::undo()
{
	UIDesigner::get()->select_elements(prev_elements, true);
}

// --------------------------------------------------------------------------------------------------

ReparentWidgetCommand::ReparentWidgetCommand(
	const Unigine::Vector<UI::ElementPtr> &in_elements, const UI::ElementPtr &in_parent_element)
{
	elements = in_elements;
	for (int i = 0; i < elements.size(); i++)
	{
		UI::ElementPtr parent = elements[i]->getParent()->getPtr();
		elements_parents.append(parent);
		elements_indices.append(parent ? parent->getChildIndex(elements[i].get()) : -1);
	}
	new_parent_element = in_parent_element;
}

void ReparentWidgetCommand::apply()
{
	for (int i = 0; i < elements.size(); i++)
	{
		vec4 t = elements[i]->getWorldPosition();
		new_parent_element->addChild(elements[i].get());
		elements[i]->setWorldPosition(t);
	}
}

void ReparentWidgetCommand::undo()
{
	for (int i = 0; i < elements.size(); i++)
	{
		vec4 t = elements[i]->getWorldPosition();
		elements_parents[i]->addChild(elements[i].get());
		elements_parents[i]->setChildIndex(elements[i].get(), elements_indices[i]);
		elements[i]->setWorldPosition(t);
	}
}

// --------------------------------------------------------------------------------------------------

ReorderWidgetCommand::ReorderWidgetCommand(const Unigine::Vector<UI::ElementPtr> &in_elements,
	const UI::ElementPtr &in_ref_element, bool in_place_before)
{
	elements = in_elements;
	for (int i = 0; i < elements.size(); i++)
	{
		UI::ElementPtr parent = elements[i]->getParent()->getPtr();
		elements_parents.append(parent);
		elements_indices.append(parent ? parent->getChildIndex(elements[i].get()) : -1);
	}
	ref_element = in_ref_element;
	place_before = in_place_before;
}

void ReorderWidgetCommand::apply()
{
	int next_index = 0;
	UI::ElementPtr ref_parent = ref_element->getParent()->getPtr();
	for (int i = 0; i < elements.size(); i++)
	{
		vec4 t = elements[i]->getWorldPosition();
		ref_parent->addChild(elements[i].get());
		if (i == 0)
		{
			int ref_element_index = ref_parent->getChildIndex(ref_element.get());
			if (place_before)
				next_index = ref_element_index;
			else
				next_index = ref_element_index + 1;
		}
		ref_parent->setChildIndex(elements[i].get(), next_index);
		next_index++;
		elements[i]->setWorldPosition(t);
	}
}

void ReorderWidgetCommand::undo()
{
	for (int i = 0; i < elements.size(); i++)
	{
		vec4 t = elements[i]->getWorldPosition();
		elements_parents[i]->addChild(elements[i].get());
		elements_parents[i]->setChildIndex(elements[i].get(), elements_indices[i]);
		elements[i]->setWorldPosition(t);
	}
}

// --------------------------------------------------------------------------------------------------

ChangedWidgetCommand::ChangedWidgetCommand(const UI::ElementPtr &in_element)
{
	prev_states.append(Blob::create());
	now_states.append(Blob::create());

	elements.append(in_element);
	in_element->getProperty()->saveState(prev_states.last());
}

ChangedWidgetCommand::ChangedWidgetCommand(const Unigine::Vector<UI::ElementPtr> &in_elements)
{
	for (int i = 0; i < in_elements.size(); i++)
	{
		prev_states.append(Blob::create());
		now_states.append(Blob::create());

		elements.append(in_elements[i]);
		in_elements[i]->getProperty()->saveState(prev_states.last());
	}
}

void ChangedWidgetCommand::saveState()
{
	for (int i = 0; i < elements.size(); i++)
	{
		now_states[i]->clear();
		elements[i]->getProperty()->saveState(now_states[i]);
	}
}

void ChangedWidgetCommand::apply()
{
	for (int i = 0; i < elements.size(); i++)
	{
		now_states[i]->seekSet(0);
		elements[i]->getProperty()->restoreState(now_states[i]);
	}
	UIDesigner::get()->refresh_selected_element_changes();
}

void ChangedWidgetCommand::undo()
{
	for (int i = 0; i < elements.size(); i++)
	{
		prev_states[i]->seekSet(0);
		elements[i]->getProperty()->restoreState(prev_states[i]);
	}
	UIDesigner::get()->refresh_selected_element_changes();
}

// --------------------------------------------------------------------------------------------------

DeleteWidgetCommand::DeleteWidgetCommand(const UI::ElementPtr &in_element)
{
#ifdef EDITOR_PLUGIN
	action = new UnigineEditor::RemoveNodesAction(in_element->getNode());
#endif

	parents.append(in_element->getNode()->getParent());
	elements.append(in_element);
}

DeleteWidgetCommand::DeleteWidgetCommand(const Unigine::Vector<UI::ElementPtr> &in_elements)
{
#ifdef EDITOR_PLUGIN
	Vector<NodePtr> nodes;
	for (int i = 0; i < in_elements.size(); i++)
		nodes.append(in_elements[i]->getNode());
	action = new UnigineEditor::RemoveNodesAction(nodes);
#endif

	elements = in_elements;
	for (int i = 0; i < elements.size(); i++)
		parents.append(elements[i]->getNode()->getParent());
}

DeleteWidgetCommand::~DeleteWidgetCommand()
{
#ifdef EDITOR_PLUGIN
	delete action;
	action = nullptr;
#else
	for (int i = 0; i < elements.size(); i++)
	{
		auto &e = elements[i];
		if (e && e->getNode()
			&& e->getNode()->getParent() == UIDesigner::get()->trash_root->getNode())
			e->getNode().deleteLater();
	}
#endif
}

void DeleteWidgetCommand::apply()
{
#ifdef EDITOR_PLUGIN
	UnigineEditor::Selection::setSelector(UnigineEditor::SelectorNodes::createObjectsSelector({}));
	action->redo();
#else
	for (int i = 0; i < elements.size(); i++)
	{
		NodePtr n = elements[i]->getNode();
		n->setParent(UIDesigner::get()->trash_root->getNode());
		n->setSaveToWorldEnabledRecursive(false);
		n->setShowInEditorEnabledRecursive(false);
		elements[i]->applyNodeHierarchyChanges(false);
	}
#endif
	UIDesigner::get()->select_element(UI::ElementPtr(), false);
}

void DeleteWidgetCommand::undo()
{
#ifdef EDITOR_PLUGIN
	action->undo();
#else
	for (int i = 0; i < elements.size(); i++)
	{
		NodePtr n = elements[i]->getNode();
		n->setParent(parents[i]);
		n->setSaveToWorldEnabledRecursive(true);
		n->setShowInEditorEnabledRecursive(true);
		elements[i]->applyNodeHierarchyChanges(false);
	}
#endif
	UIDesigner::get()->select_elements(elements, true);
}

// --------------------------------------------------------------------------------------------------
