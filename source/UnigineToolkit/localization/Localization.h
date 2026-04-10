// Copyright (C), UNIGINE. All rights reserved.
#pragma once
#include <UnigineEvent.h>

class Localization
{
public:
	// clears all localization data
	static void clear();

	// load localization (.csv files)
	static void appendDirectory(const char *directory_path);
	static void appendFile(const char *file_path);

	// languages info
	static int getNumLanguages();
	static const char *getLanguage(int index);

	// set/get current language
	static void setLanguage(const char *name);
	static const char *getLanguage();

	// get localized string
	static const char *get(const char *key);

	// events
	static Unigine::Event<> &getEventLanguageChanged();
};
