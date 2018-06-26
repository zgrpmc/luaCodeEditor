#include "ScriptManager.h"
#include <string>
#include "luacodeTolua.h"

ScriptManager::ScriptManager() : m_luaEngine(NULL)
{

}

ScriptManager::~ScriptManager()
{

}

void ScriptManager::init(const char * workpath)
{
	m_workpath = workpath;
	//��ʼ��lua����
	m_luaEngine = CCLuaEngine::defaultEngine();
	tolua_luacode_open(m_luaEngine->getLuaStack()->getLuaState());

	//ִ�е�һ��lua�ļ�
	dofile("init.lua");
}

void ScriptManager::dofile(const char * path)
{
	std::string file = m_workpath + path;
	m_luaEngine->executeScriptFile(file.c_str());
}

void ScriptManager::dostring(const char * str)
{
	m_luaEngine->executeString(str);
}
