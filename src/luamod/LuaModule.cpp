#include "LuaModule.h"
#include <stdexcept>
#include "util/util_log.h"
#include "util/util_string.h"

extern "C" int luaopen_sdplua(lua_State *L);

namespace mfw
{

string LuaValue::toString() const {
    ostringstream os;
    toString(os);

    return os.str();
}

void LuaValue::toString(ostringstream &os, uint32_t level) const {
    switch (m_type) {
        case LuaValueType_Nil: {
            os << "nil";
        }break;
        case LuaValueType_Boolean: {
            os << m_valBool;
        }break;
        case LuaValueType_Number: {
            os << m_valNumber;
        }break;
        case LuaValueType_String: {
            os << "'" << m_valString << "'";
        }break;
        case LuaValueType_Table: {
            os << "{" << endl;
            for (map<LuaValue,LuaValue>::const_iterator it = m_valTable.begin(); it != m_valTable.end(); ++it) {
                for (uint32_t i = 0; i < level; ++i) {
                    os << "    ";
                }
                it->first.toString(os, level+1);
                os << " = ";
                it->second.toString(os, level+1);
                os << endl;
            }
            for (uint32_t i = 1; i < level; ++i) {
                os << "    ";
            }
            os << "}";
        }break;
    }
}

LuaModule::LuaModule()
	: L(NULL), m_bOwned(true)
{
	L = lua_open();
	if (L == NULL)
	{
		throw std::runtime_error("cannot alloc new lua state");
	}

	luaL_openlibs(L);
	luaopen_sdplua(L);
}

LuaModule::LuaModule(lua_State *LL, bool bOwned)
	: L(LL), m_bOwned(bOwned)
{
}

LuaModule::~LuaModule()
{
	if (m_bOwned && L != NULL)
	{
		lua_close(L);
		L = NULL;
	}
}

void LuaModule::modifyPackagePath(const string &sPath, const char *key, bool bAdd)
{
	vector<string> vNewPath = UtilString::splitString(sPath, ";", true);

	lua_getglobal(L, "package");
	if (bAdd)
	{
		lua_getfield(L, -1, key);
		string sOldPath = tostring(-1);
		pop(1);

		vector<string> vOldPath = UtilString::splitString(sOldPath, ";", true);
		vNewPath.insert(vNewPath.end(), vOldPath.begin(), vOldPath.end());
	}

	for (unsigned i = 1; i < vNewPath.size(); )
	{
		bool bFound = false;
		for (unsigned j = 0; j < i; ++j)
		{
			if (vNewPath[j] == vNewPath[i])
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			vNewPath.erase(vNewPath.begin() + i);
		}
		else
		{
			++i;
		}
	}

	string sNewPath = UtilString::joinString(vNewPath, ";");
	lua_pushlstring(L, sNewPath.c_str(), sNewPath.size());
	lua_setfield(L, -2, key);
	pop(1);
}

void LuaModule::addPath(const string &sPath)
{
	modifyPackagePath(sPath, "path", true);
}

void LuaModule::setPath(const string &sPath)
{
	modifyPackagePath(sPath, "path", false);
}

void LuaModule::addCPath(const string &sCPath)
{
	modifyPackagePath(sCPath, "cpath", true);
}

void LuaModule::setCPath(const string &sCPath)
{
	modifyPackagePath(sCPath, "cpath", false);
}

int LuaModule::gettop()
{
	return lua_gettop(L);
}

void LuaModule::settop(int idx)
{
	lua_settop(L, idx);
}

string LuaModule::tostring(int idx)
{
	size_t len = 0;
	const char *p = lua_tolstring(L, idx, &len);
	if (p != NULL && len != 0)
	{
		return string(p, len);
	}
	return "";
}

bool LuaModule::toboolean(int idx)
{
	return lua_toboolean(L, idx);
}

int32_t LuaModule::tointeger(int idx)
{
    return lua_tointeger(L, idx);
}

double LuaModule::tonumber(int idx)
{
    return lua_tonumber(L, idx);
}

void LuaModule::createtable(int narr, int nrec) {
    lua_createtable(L, narr, nrec);
}

int LuaModule::pcall(int nargs, int nresults, int errfunc)
{
	int ret = lua_pcall(L, nargs, nresults, errfunc);
	if (ret != 0)
	{
		return ret;
	}
	return 0;
}

bool LuaModule::doFile(const string &sFileName, int nresults)
{
	int ret = luaL_loadfile(L, sFileName.c_str());
	if (ret != 0)
	{
		LOG_ERROR("cannot load file: " << sFileName);
		return false;
	}

	ret = pcall(0, nresults);
	if (ret != 0)
	{
		LOG_ERROR("fail to doFile: " << sFileName << ", msg: " << tostring(-1));
		return false;
	}
	return true;
}

bool LuaModule::doString(const string &sScript, int nresults)
{
	int ret = luaL_loadbuffer(L, sScript.c_str(), sScript.size(), "script");
	if (ret != 0)
	{
		LOG_ERROR("cannot load buffer: " << sScript);
		return false;
	}

	ret = pcall(0, nresults);
	if (ret != 0)
	{
		LOG_ERROR("fail to doString: " << sScript << ", msg: " << tostring(-1));
		pop(1);
		return false;
	}
	return true;
}

void LuaModule::pop(int n)
{
	lua_pop(L, n);
}

void LuaModule::push(bool v)
{
	lua_pushboolean(L, v);
}

void LuaModule::push(int32_t v)
{
	lua_pushinteger(L, v);
}

void LuaModule::push(uint32_t v)
{
    push((double)v);
}

void LuaModule::push(int64_t v)
{
	push(UtilString::tostr(v));
}

void LuaModule::push(uint64_t v)
{
	push(UtilString::tostr(v));
}

void LuaModule::push(double v)
{
	lua_pushnumber(L, v);
}

void LuaModule::push(const string &v)
{
	lua_pushlstring(L, v.c_str(), v.size());
}

void LuaModule::push(const char* v)
{
	lua_pushstring(L, v);
}

void LuaModule::push(const LuaValue &v) {
    if (v.isNil()) {
		throw std::runtime_error("cannot push nil value");
    }

    if (v.isBoolean()) {
        push(v.m_valBool);
        return;
    }

    if (v.isNumber()) {
        push(v.m_valNumber);
        return;
    }

    if (v.isString()) {
        push(v.m_valString);
        return;
    }

    push(v.m_valTable);
}

void LuaModule::peek(int idx, int64_t &v)
{
    v = UtilString::strto<int64_t>(tostring(idx));
}

void LuaModule::peek(int idx, uint64_t &v)
{
    v = UtilString::strto<uint64_t>(tostring(idx));
}

void LuaModule::peek(int idx, LuaValue &v) {
    int type = lua_type(L, idx);
    if (type == LUA_TNIL) {
        return;
    }

    if (type == LUA_TBOOLEAN) {
        bool b;
        peek(idx, b);
        v.set(b);
        return;
    }

    if (type == LUA_TNUMBER) {
        double d;
        peek(idx, d);
        v.set(d);
        return;
    }

    if (type == LUA_TSTRING) {
        v.set(tostring(idx));
        return;
    }

    if (type == LUA_TTABLE) {
        v.m_type = LuaValue::LuaValueType_Table;
        peek(idx, v.m_valTable);
        return;
    }
}

void LuaModule::registerlib(const char *libname, const luaL_Reg *reg)
{
	luaL_register(L, libname, reg);
}

static int _mfwlog_roll_impl(lua_State *L, MfwLogLevel level)
{
	LuaModule luamod(L, false);
	string sData = luamod.tostring(-1);
	switch (level)
	{
	case MfwLogLevel_Debug: LOG_DEBUG(sData); break;
	case MfwLogLevel_Info: LOG_INFO(sData); break;
	case MfwLogLevel_Error: LOG_ERROR(sData); break;
	case MfwLogLevel_None: break;
	}
	return 0;
}

static int _mfwlog_debug(lua_State *L)
{
	return _mfwlog_roll_impl(L, MfwLogLevel_Debug);
}

static int _mfwlog_info(lua_State *L)
{
	return _mfwlog_roll_impl(L, MfwLogLevel_Info);
}

static int _mfwlog_error(lua_State *L)
{
	return _mfwlog_roll_impl(L, MfwLogLevel_Error);
}

static int _mfwlog_file_impl(lua_State *L, MfwLogType type)
{
	LuaModule luamod(L, false);
	string sData = luamod.tostring(-1);
	string sFile = luamod.tostring(-2);
	switch (type)
	{
	case MfwLogType_Roll: break;
	case MfwLogType_Day: DAYLOG(sFile, sData); break;
	case MfwLogType_Hour: HOURLOG(sFile, sData); break;
	default: break;
	}
	return 0;
}

static int _mfwlog_daylog(lua_State *L)
{
	return _mfwlog_file_impl(L, MfwLogType_Day);
}

static int _mfwlog_hourlog(lua_State *L)
{
	return _mfwlog_file_impl(L, MfwLogType_Hour);
}

static const luaL_Reg _mfwlog[] = {
	{"debug", _mfwlog_debug},
	{"info", _mfwlog_info},
	{"error", _mfwlog_error},
	{"daylog", _mfwlog_daylog},
	{"hourlog", _mfwlog_hourlog},
	{NULL, NULL}
};

void LuaModule::registerMfwLog(const char *libname)
{
	registerlib(libname, _mfwlog);
}

int LuaModule::traceback(lua_State *L) {

    string err;

    size_t len = 0;
	const char *p = lua_tolstring(L, -1, &len);
	if (p != NULL && len != 0)
	{
		err = string(p, len);
	}

    if (!err.empty()) {
        err += "\n";
    }

    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    if (lua_pcall(L,0,1,0) == 0) {
	    const char *p2 = lua_tolstring(L, -1, &len);
	    if (p2 != NULL && len != 0)
	    {
	    	err += string(p2, len);
	    }
    }

	lua_pushlstring(L, err.c_str(), err.size());

    return 1;
}

}
