/*

Decoda
Copyright (C) 2007-2013 Unknown Worlds Entertainment, Inc. 

This file is part of Decoda.

Decoda is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Decoda is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Decoda.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "AutoCompleteManager.h"
#include "Symbol.h"

#include <algorithm>

std::hash<std::string> AutoCompleteManager::hashFN;

AutoCompleteManager::Entry::Entry()
{
}

AutoCompleteManager::Entry::Entry(const wxString& _name, Type _type, const wxString& _scope)
    : name(_name), type(_type), scope(_scope)
{
    lowerCaseName = name.Lower();
	hashVal = hashFN((scope+name).c_str());
}

bool AutoCompleteManager::Entry::operator<(const Entry& entry) const
{
	return hashVal < entry.hashVal;
}

bool AutoCompleteManager::ApiEntry::operator<(const ApiEntry& entry) const
{
	int cmpClassValue = this->className.Cmp(entry.className);
	if (cmpClassValue == 0)
		return this->funcName.Cmp(entry.funcName) < 0;
	return cmpClassValue < 0;
}


void AutoCompleteManager::clear()
{
	m_entries.clear();
	m_symbols.clear();
}

void AutoCompleteManager::RebuildFromProject(const Project* project)
{
    m_entries.clear();
	m_symbols.clear();

    for (unsigned int fileIndex = 0; fileIndex < project->GetNumFiles(); ++fileIndex)
    {
        BuildFromFile(project->GetFile(fileIndex));
    }
}

void AutoCompleteManager::BuildFromFile(const Project::File* file)
{
    for (unsigned int symbolIndex = 0; symbolIndex < file->symbols.size(); ++symbolIndex)
    {
        const Symbol* symbol = file->symbols[symbolIndex];
//		m_entries.insert(Entry(symbol->name, Type_Function, symbol->module));
		m_symbols.push_back(*symbol);
    }

}

void AutoCompleteManager::loadAllAPIs()
{
	char curDir[1000] = { 0 };
	GetModuleFileNameA(NULL, curDir, sizeof(curDir)-1);
	char *findpos = strrchr(curDir, '\\');
	if (findpos != NULL)*findpos = 0;

	WIN32_FIND_DATA findData;
	memset(&findData, 0, sizeof(findData));
	HANDLE hFind = FindFirstFileA(wxString(curDir) + "\\Apis\\*.api", &findData);
	do
	{
		if ((findData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			loadAPIs(wxString(curDir) + "\\Apis\\" + findData.cFileName);
		}
	} while (FindNextFileA(hFind, &findData));
	FindClose(hFind);
}

void AutoCompleteManager::loadAPIs(const char *path)
{
	FILE *fp = fopen(path, "rt");
	if (!fp)return;
	m_apis.clear();

	while (!feof(fp))
	{
		char line[1024] = { 0 };
		fgets(line, sizeof(line), fp);

		wxString text = line;
		text.Replace("\n", "");
		text.Replace("\r", "");
		text.Replace("\t", " ");
		text.Trim();
		text.Trim(false);
		if (text.size() < 1)
			continue;

		if (strncmp(text, "--", 2) == 0 || strncmp(text, "//", 2) == 0)
			continue;

		ApiEntry item;
		item.text = text;

		int npos1 = text.Find('.');
		int npos2 = text.Find(':');
		int npos = npos1 >= 0 ? npos1 : npos2;
		wxString className, funcName;
		if (npos>=0)
		{
			className = text.Left(npos);
			funcName = text.Mid(npos + 1);
		}
		else
		{
			funcName = text;
		}

		if (className.size() > 0)
		{
			npos2 = className.Find(' ');
			if (npos2 > 0)
			{
				item.returnName = className.Left(npos2);
				className.Remove(0, npos2);
				className.Trim(false);
			}
			item.className = className;
		}

		if (funcName.size() > 0)
		{
			npos2 = funcName.Find('(');
			if (npos2 > 0)
			{
				funcName = funcName.Left(npos2);
			}
			if (className.size() <= 0)
			{
				npos2 = funcName.Find(' ');
				if (npos2 > 0)
				{
					item.returnName = funcName.Left(npos2);
					funcName.Remove(0, npos2);
					funcName.Trim(false);
				}
			}
			item.funcName = funcName;
		}
		if (item.funcName.size() > 0)
		{
			item.hashVal = hashFN((item.className + "." + item.funcName).c_str());
			m_apis.insert(item);
		}
	}
	fclose(fp);
}

static int calSequenceScore(const char * p, const char * c)
{
	int i = 0;
	int j = 0;
	int score = 1;
	while(c[i]){
		if(!p[j]){
			return false;
		}
		if(p[j] == c[i]){
			++i;
			++j;
		}else{
			score += 100;
			++j;
		}
	}
	while(p[j++]){
		score++;
	}
	return score;
}

struct StringScore{
	const AutoCompleteManager::Entry * entry;
	int score;
public:
	StringScore(){}
	StringScore(const AutoCompleteManager::Entry * entry_, int score_){
		entry = entry_;
		score = score_;
	}
	bool operator < (const StringScore & sc) const{
		return this->score < sc.score;
	}
};



void AutoCompleteManager::GetMatchingItems(const wxString& module, const wxString& prefix, bool member, wxString& items) const
{
	GetMatchingItems2(module, prefix, member, items);
	return;

    // Autocompletion selection is case insensitive so transform everything
    // to lowercase.
    wxString test = prefix.Lower();
	
    // Add the items to the list that begin with the specified prefix. This
    // could be done much fater with a binary search since our items are in
    // alphabetical order.
	std::multiset<StringScore> scores;

	for (ENTRY_ITR itr = m_entries.begin(); itr != m_entries.end(); ++itr)
    {
        // Check that the scope is correct.
        
        bool inScope = false;
        if (/**/true)
        {
            // We've got no way of knowing the type of the variable in Lua (since
            // variables don't have types, only values have types), so we display
            // all members if the prefix contains a member selection operator (. or :)
			inScope = (itr->scope.IsEmpty() != member);
        }

		//if (inScope &&  itr->lowerCaseName.StartsWith(test) && itr->scope == module)
		int score = calSequenceScore(itr->lowerCaseName, test);
		if (inScope && score != 0 && itr->scope == module)
		{
			const Entry  & entry = *itr;
			scores.insert(StringScore(&entry, score));
        }
    }

	for (std::set<StringScore>::iterator itr = scores.begin(); itr != scores.end(); ++itr){

		items += itr->entry->name;
		// Add the appropriate icon for the type of the identifier.
		if (itr->entry->type != Type_Unknown)
		{
			items += "?";
			items += '0' + itr->entry->type;
		}

		items += ' ';
	}
}


struct ApiStringScore{
	const AutoCompleteManager::ApiEntry * entry;
	int score;
	int type;
public:
	ApiStringScore(){}
	ApiStringScore(const AutoCompleteManager::ApiEntry * entry_, int score_, int type_){
		entry = entry_;
		score = score_;
		type = type_;
	}
	const wxString& name()const{
		return type == 0 ? entry->funcName : entry->className;
	}
	bool operator < (const ApiStringScore & sc) const{
		return this->score < sc.score;
	}
};


static int calSequenceScore2(const char * p, const char * c)
{
	int i = 0;
	int score = 1;
	while (c[i] && p[i] && c[i] == p[i])i++;

	if (c[i] != 0)
		return 0;
	while (p[i++]){
		score++;
	}
	return score;
}


void AutoCompleteManager::GetMatchingItems2(const wxString& module0, const wxString& prefix, bool member, wxString& items) const
{
	wxString module = module0;
	int pos1 = module0.Find(':',true);
	if (pos1 >= 0)
		module = module0.Mid(pos1 + 1);
	
	if (module.size() > 0)
	{
		module = findApiClass(wxString(module));
	}

    // Autocompletion selection is case insensitive so transform everything
    // to lowercase.
    wxString test = prefix.Lower();
	
    // Add the items to the list that begin with the specified prefix. This
    // could be done much fater with a binary search since our items are in
    // alphabetical order.
	std::multiset<ApiStringScore> scores;

	for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
    {
		//if (inScope &&  itr->lowerCaseName.StartsWith(test) && itr->scope == module)
		int score = -1, type = 0;
		if (module.size() > 0)
		{
			if (module.Cmp(itr->className) == 0)
				score = calSequenceScore2(itr->funcName.Lower(), test);
		}
		else
		{
			if (itr->className.size() == 0)
				score = calSequenceScore2(itr->funcName.Lower(), test);
			else
				score = calSequenceScore2(itr->className.Lower(), test), type = 1;
		}
		if (score>0)
		{
			const ApiEntry  & entry = *itr;
			auto curItem = ApiStringScore(&entry, score, type);

			bool findSame = false;
			for (auto s_itr = scores.begin(); s_itr != scores.end(); s_itr++)
			{
				if (s_itr->type == type && s_itr->name().Lower() == curItem.name().Lower())
				{
					findSame = true;
					break;
				}
			}
			if (!findSame)
				scores.insert(curItem);
        }
    }

	if (scores.size() > 0)
	{
		for (std::set<ApiStringScore>::iterator itr = scores.begin(); itr != scores.end(); ++itr){

			items += itr->name();
			// Add the appropriate icon for the type of the identifier.		
			{
				items += "?";
				items += '0' + (itr->type == 1 ? Type_Class : Type_Function);
			}

			items += ' ';
		}
		return;
	}
	//无法检测出来（可能来自于赋值、参数传入），只好提示全部class
	else if (module0.size()>0)
	{
		std::vector<wxString> classNames;
		for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
		{
			if (itr->className.size()!= 0)
			{
				if (std::find(classNames.begin(), classNames.end(), itr->className) == classNames.end())
				{
					if (test.size() == 0 || itr->className.Lower().Find(test) == 0)
						classNames.push_back(itr->className);
				}
			}
		}
		for (int i = 0; i < classNames.size(); i++)
		{
			items += classNames[i];
			items += "?";
			items += '0' + (Type_Class);
			items += ' ';
		}
	}

	//以上匹配综合考虑了模块名和函数名，找不到匹配项的话，就再单找函数匹配项
	if (items.size() == 0)
	{
		std::vector<wxString> funcNames;
		for (auto itr = m_symbols.begin(); itr != m_symbols.end(); ++itr)
		{
			if ((itr->type == Symbol::SymbolFunction || itr->type == Symbol::SymbolTableFunction) && itr->name.Lower().Find(test) == 0)
			{
				if (std::find(funcNames.begin(), funcNames.end(), itr->name)==funcNames.end())
					funcNames.push_back(itr->name);
			}
		}

		for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
		{
			if (itr->funcName.Lower().Find(test) == 0)
			{
				if (std::find(funcNames.begin(), funcNames.end(), itr->funcName)== funcNames.end())
					funcNames.push_back(itr->funcName);
			}
		}
		for (int i = 0; i < funcNames.size(); i++)
		{
			items += funcNames[i];
			items += "?";
			items += '0' + (Type_Function);
			items += ' ';
		}
	}
}

wxString AutoCompleteManager::findApiClass(const wxString& module0)const
{
	wxString module = module0;
	if (module.size() > 0)
	{
		bool findModule = false;
		for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
		{
			if (module.Cmp(itr->className) == 0)
			{
				findModule = true;
				break;
			}
		}

		if (!findModule)
		{
			//是否属于类型变量
			for (auto itr = m_symbols.begin(); itr != m_symbols.end(); ++itr)
			{
				//返回类型
				if (itr->type == Symbol::SymbolReturnIdentifier && itr->name == module && (itr + 1) != m_symbols.end())
				{
					wxString className;
					auto itr1 = (itr + 1);
					if (itr1->type == Symbol::SymbolTable && (itr1 + 1) != m_symbols.end() && (itr1 + 1)->type == Symbol::SymbolTableFunction)
						className = findApiReturnType(findApiClass(itr1->name), (itr1 + 1)->name);
					else if (itr1->type == Symbol::SymbolFunctionCall)
						className = findApiReturnType("", itr1->name);

					//将该类的成员函数作为提示显示出来
					if (className.size() > 0)
					{
						module = className;
						findModule = true;
					}
					break;
				}
			}
		}
	}
	return module;
}

wxString AutoCompleteManager::findApiTip(const wxString& module0, const wxString& prefix)const
{
	wxString test = prefix.Lower();
	wxString module = findApiClass(module0);

	for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
	{
		if (module.Cmp(itr->className) == 0 && prefix.Cmp(itr->funcName) == 0)
			return itr->text;
	}

	wxString text;
	for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
	{
		if (prefix.Cmp(itr->funcName) == 0)
		{
			text += itr->text + "\r\n";
		}
	}

	for (auto itr = m_symbols.begin(); itr != m_symbols.end(); ++itr)
	{
		if ((itr->type == Symbol::SymbolFunction || itr->type == Symbol::SymbolTableFunction) && prefix.Cmp(itr->name) == 0)
		{
			if (itr->func_definiton.size()>0)
				text += itr->func_definiton + "\r\n";
		}
	}

	return text;
}

wxString AutoCompleteManager::findApiReturnType(const wxString& module, const wxString& func)const
{
	if (module.size() == 0)
	{
		for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
		{
			if (itr->className.size() == 0 && func.Cmp(itr->funcName) == 0)
				return itr->returnName;
		}
		for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
		{
			if (func.Cmp(itr->className) == 0)
				return itr->className;
		}
	}
	else
	{
		for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
		{
			if (module.Cmp(itr->className) == 0 && func.Cmp(itr->funcName) == 0)
				return itr->returnName;
		}
	}

	return wxString();
}


bool AutoCompleteManager::cutClassNameInFunctionNames(wxString& text)const
{
	int pos1 = text.Find('.');
	int pos2 = text.Find(':');
	int pos = (pos1 >= 0 && pos2 >= 0) ? std::min(pos1, pos2) : std::max(pos1, pos2);
	if (pos < 0)
		return false;

	wxString newText = text.Left(pos);

	int pos0 = pos;
	int start = pos;
	while (pos <= text.size())
	{
		if (pos==text.size() || text[pos] == '.' || text[pos] == ':' || text[pos] == 0 || text[pos] == '\r' || text[pos] == '\n')
		{
			wxString name = text.Mid(start + 1, pos - start - 1);
			if (name.size()>0)
			{
				bool findClass = false;
				for (API_ENTRY_ITR itr = m_apis.begin(); itr != m_apis.end(); ++itr)
				{
					if (name.Cmp(itr->className) == 0)
					{
						findClass = true;
						break;
					}
				}
				if (!findClass)
				{
					newText += text.Mid(start, pos - start);
				}
			}
			start = pos;
		}
		pos++;
	}
	text = newText;
	return true;
}

void AutoCompleteManager::addEntry(const char *name, Type type, const char *scope /*= ""*/)
{
	m_entries.insert(Entry(name, type, scope));
}

AutoCompleteManager::AutoCompleteManager()
{
	addEntry("function", Type_Function, "");
	addEntry("module", Type_Function, "");
}
