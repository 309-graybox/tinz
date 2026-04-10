#include "LoadLocalization.h"

#include "../../localization/Localization.h"
#include "../../ui/elements/Canvas.h"

using namespace Unigine;
using namespace Math;
using namespace UI;

REGISTER_COMPONENT(LoadLocalization);

void LoadLocalization::init()
{
	if (localization_file.nullCheck() == 0)
	{
		Localization::appendFile(localization_file);
		Localization::setLanguage(set_language);
	}
}

void LoadLocalization::clear()
{
	Localization::clear();
	refresh_ui();
}

void LoadLocalization::appendDirectory(const char *directory_path)
{
	Localization::appendDirectory(directory_path);
	refresh_ui();
}

void LoadLocalization::appendFile(const char *file_path)
{
	Localization::appendFile(file_path);
	refresh_ui();
}

int LoadLocalization::getNumLanguages()
{
	return Localization::getNumLanguages();
}

const char *LoadLocalization::getLanguage(int index)
{
	return Localization::getLanguage();
}

void LoadLocalization::setLanguage(const char *name)
{
	Localization::setLanguage(name);
	refresh_ui();
}

const char *LoadLocalization::getLanguage()
{
	return Localization::getLanguage();
}

const char *LoadLocalization::get(const char *key)
{
	return Localization::get(key);
}

Unigine::Event<> &LoadLocalization::getEventLanguageChanged()
{
	return Localization::getEventLanguageChanged();
}

void LoadLocalization::refresh_ui()
{
	auto &c = Canvas::getAllCanvases();
	for (int i = 0; i < c.size(); i++)
		c[i]->applyPropertyChangesRecursively();
}
