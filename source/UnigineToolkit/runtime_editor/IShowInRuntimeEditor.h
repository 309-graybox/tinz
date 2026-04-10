// Copyright (C), UNIGINE. All rights reserved.
#pragma once
#include "../imgui/imgui/imgui.h"

// Interface for Unigine::ComponentBase.
// It allows you to show some debug info
// into the Node Window in the RuntimeEditor

class IShowInRuntimeEditor
{
public:
	virtual void editorUpdate() = 0;

protected:
	~IShowInRuntimeEditor() = default;
};
