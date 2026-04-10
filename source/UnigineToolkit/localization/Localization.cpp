// Copyright (C), UNIGINE. All rights reserved.
#include "Localization.h"

#include <UnigineDir.h>
#include <UnigineFileSystem.h>
#include <UnigineHashMap.h>
#include <UnigineStreams.h>
#include <UnigineString.h>
#include <UnigineVector.h>

using namespace Unigine;
using namespace Math;

namespace {
int cur_lang = -1;	  // language index in languages
Vector<String> languages;
typedef HashMap<String /*key*/, Vector<String> /*values (in all languages)*/> Data;
Data localization_data;
EventInvoker<> language_changed_event;

void merge_localization_data(Data &data)
{
	// erase first header line, get languages list
	const Vector<String> data_languages = data.take("key");

	// add new language names
	for (const auto &code : data_languages)
		languages.appendUnique(code);

	// compare order of language lists
	if (data_languages == languages)
	{
		// easy situation. Columns are equal to our merged version
		for (auto it = data.begin(); it != data.end(); ++it)
			localization_data.append(it->key, it->data);
	}
	else
	{
		// hard situation. Columns are different to our merged version
		Vector<int> mask;
		mask.resize(languages.size());
		for (int j = 0; j < mask.size(); j++)
			mask[j] = data_languages.findIndex(languages[j]);

		for (auto it = data.begin(); it != data.end(); ++it)
		{
			Vector<String> value;
			for (int m : mask)
			{
				if (m != -1 && m < it->data.size())
					value.append(it->data[m]);
				else
					value.append();
			}
			localization_data.append(it->key, value);
		}
	}
}

// csv parser
Vector<String> read_csv_row(const String &row)
{
	enum class CSVState { UnquotedField, QuotedField, QuotedQuote };
	CSVState state = CSVState::UnquotedField;
	Vector<String> fields{""};
	size_t i = 0;	 // index of the current field
	for (int j = 0; j < row.size(); j++)
	{
		char c = row[j];
		switch (state)
		{
		case CSVState::UnquotedField:
			switch (c)
			{
			case ',':	 // end of field
				fields.append();
				i++;
				break;
			case '"':
				state = CSVState::QuotedField;
				break;
			default:
				fields[i].append(c);
				break;
			}
			break;
		case CSVState::QuotedField:
			switch (c)
			{
			case '"':
				state = CSVState::QuotedQuote;
				break;
			default:
				fields[i].append(c);
				break;
			}
			break;
		case CSVState::QuotedQuote:
			switch (c)
			{
			case ',':	 // , after closing quote
				fields.append();
				i++;
				state = CSVState::UnquotedField;
				break;
			case '"':	 // "" -> "
				fields[i].append('"');
				state = CSVState::QuotedField;
				break;
			default:	// end of quote
				state = CSVState::UnquotedField;
				break;
			}
			break;
		}
	}
	return fields;
}

void parse_csv(const char *path, Data &strings)
{
	FilePtr file = File::create(path, "r");
	if (file->isOpened())
	{
		while (!file->eof())
		{
			StringStack<> row = file->readLine().trimLast("\n");
			row = row.replace("{amp}", "&amp;");
			Vector<String> fields = read_csv_row(row);
			if (!fields.size())
				continue;

			// read strings
			Vector<String> &values = strings.append(fields[0].trim());
			for (int i = 1; i < fields.size(); i++)
				values.append(fields[i].replace("\\n", "\n"));
		}
		file->close();
	}
}
}	 // namespace

void Localization::clear()
{
	cur_lang = -1;
	languages.clear();
	localization_data.clear();

	language_changed_event.run();
}

void Localization::appendDirectory(const char *directory_path)
{
	DirPtr dir = Dir::create(FileSystem::getAbsolutePath(directory_path));
	if (dir->isOpened())
	{
		// load all .csv files
		for (int i = 0; i < dir->getNumFiles(); i++)
		{
			StringStack<> file_name = dir->getFileName(i);
			if (file_name.extension() == "csv")
			{
				Data file_strings;
				parse_csv(file_name, file_strings);
				merge_localization_data(file_strings);
			}
		}
		dir->close();
	}

	language_changed_event.run();
}

void Localization::appendFile(const char *filename)
{
	Data file_strings;
	parse_csv(filename, file_strings);
	merge_localization_data(file_strings);

	language_changed_event.run();
}

int Localization::getNumLanguages()
{
	return languages.size();
}

const char *Localization::getLanguage(int index)
{
	if (!languages.isValidNum(index))
		return nullptr;
	return languages[index];
}

void Localization::setLanguage(const char *name)
{
	cur_lang = languages.findIndex(name);
	if (cur_lang == -1 && !languages.empty())
		cur_lang = 0;

	language_changed_event.run();
}

const char *Localization::getLanguage()
{
	if (cur_lang == -1)
		return nullptr;

	return languages[cur_lang];
}

const char *Localization::get(const char *key)
{
	if (cur_lang == -1 || !key || key[0] == '\0')
		return key;

	auto it = localization_data.find(key);
	if (it == localization_data.end())
		return key;

	return it->data[cur_lang];
}

Unigine::Event<> &Localization::getEventLanguageChanged()
{
	return language_changed_event;
}
