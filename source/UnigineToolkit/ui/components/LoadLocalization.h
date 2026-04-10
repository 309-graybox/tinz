#pragma once

#include <UnigineComponentSystem.h>

namespace UI {

// Note: attach this component to DrawSurface element!
class LoadLocalization : public Unigine::ComponentBase
{
public:
	COMPONENT_DEFINE(LoadLocalization, ComponentBase);
	COMPONENT_INIT(init, -1001 /*right before canvas initialization*/);

	PROP_PARAM(File, localization_file, "", "Localization File", "", "", "filter=.csv");
	PROP_PARAM(String, set_language, "en");

	void clear();

	// load localization (.csv files)
	void appendDirectory(const char *directory_path);
	void appendFile(const char *file_path);

	// languages info
	int getNumLanguages();
	const char *getLanguage(int index);

	// set/get current language
	void setLanguage(const char *name);
	const char *getLanguage();

	// get localized string
	const char *get(const char *key);

	// events
	Unigine::Event<> &getEventLanguageChanged();

protected:
	void init();
	void refresh_ui();
};
}	 // namespace UI
