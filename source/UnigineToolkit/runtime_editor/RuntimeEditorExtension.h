#pragma once

class RuntimeEditorExtension
{
public:
	virtual void processHotkeys() = 0;
	virtual void renderMenuWindows() = 0;
	virtual void renderWindow() = 0;
};
