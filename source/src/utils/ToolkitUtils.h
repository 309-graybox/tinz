#pragma once

#define GET_CANVAS(TO, FROM)             \
	TO = getComponent<UI::Canvas>(FROM); \
	if (!TO)                             \
		return;                          \
	TO->setEnabled(false);

#define GET_TABLE(FROM, NAME) dynamic_cast<UI::Table *>(FROM->findChild(#NAME));
#define GET_SPRITE(FROM, NAME) dynamic_cast<UI::Sprite *>(FROM->findChild(#NAME));
#define GET_LABEL(FROM, NAME) dynamic_cast<UI::Label *>(FROM->findChild(#NAME));

#define GET_BUTTON(FROM, NAME, METHOD)                                 \
	{                                                                  \
		auto btn = dynamic_cast<UI::Button *>(FROM->findChild(#NAME)); \
		if (btn)                                                       \
		{                                                              \
			btn->getEventButtonClicked().connect(*this, METHOD);       \
		}                                                              \
	}

#define GET_SLIDER(FROM, NAME, METHOD)                                    \
	{                                                                     \
		auto slider = dynamic_cast<UI::Slider *>(FROM->findChild(#NAME)); \
		if (slider)                                                       \
		{                                                                 \
			slider->getEventSliderChanged().connect(*this, METHOD);       \
		}                                                                 \
	}