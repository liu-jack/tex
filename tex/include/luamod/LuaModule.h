#ifndef _LUA_MODULE_H_
#define _LUA_MODULE_H_

#include <luajit-2.0/lua.hpp>
#include <tr1/tuple>
#include "sdp/Sdp.h"
#include "util/util_string.h"
using namespace std;

namespace mfw
{

struct LuaStackKeeper
{
	lua_State *L;
	int iOldTop;

	explicit LuaStackKeeper(lua_State *LL) : L(LL), iOldTop(lua_gettop(L)) {}
	~LuaStackKeeper() { lua_settop(L, iOldTop); }
};

#define KEEP_LUA_STACK_X(l) LuaStackKeeper _dummy_lua_stack_keeper(l)
#define KEEP_LUA_STACK() KEEP_LUA_STACK_X(L)

namespace impl
{
template <typename T> struct LuaValueCast
{
};
}

class LuaValue
{
public:
	enum LuaValueType
	{
		LuaValueType_Nil,
		LuaValueType_Boolean,
		LuaValueType_Number,
		LuaValueType_String,
		LuaValueType_Table,
	};

	// 实现类似jquery的链式操作，add进入下一层，prev返回上一层
	struct LuaValueProxy
	{
		LuaValueProxy *m_prev;
		LuaValue *m_parent;
		LuaValue *m_data;

		LuaValueProxy() : m_prev(NULL), m_parent(NULL), m_data(NULL) {}

		template <typename T>
		LuaValueProxy add(const T &k)
		{
			LuaValueProxy prx = m_data->add(k);
			prx.m_prev = this;
			return prx;
		}

		template <typename T>
		LuaValueProxy &set(const T &v)
		{
			m_data->set(v);
			return *this;
		}

		template <typename T, typename T2>
		LuaValueProxy &set(const T &k, const T2 &v)
		{
			m_data->set(k, v);
			return *this;
		}

		LuaValueProxy prev()
		{
			if (m_prev != NULL)
			{
				return *m_prev;
			}
			if (m_parent != NULL)
			{
				LuaValueProxy prx;
				prx.m_data = m_parent;
				return prx;
			}
			return *this;
		}

		operator LuaValue& ()
		{
			return *m_data;
		}
	};

public:
	typedef map<LuaValue, LuaValue>::iterator iterator;
	typedef map<LuaValue, LuaValue>::const_iterator const_iterator;

	size_t size() const { return m_valTable.size(); }
	iterator begin() { return m_valTable.begin(); }
	const_iterator begin() const { return m_valTable.begin(); }
	iterator end() { return m_valTable.end(); }
	const_iterator end() const { return m_valTable.end(); }

public:
	bool operator<(const LuaValue &b) const
	{
		if (m_type != b.m_type)
			return m_type < b.m_type;
		switch (m_type)
		{
		case LuaValueType_Nil: return false;
		case LuaValueType_Boolean: return m_valBool < b.m_valBool;
		case LuaValueType_Number: return m_valNumber < b.m_valNumber;
		case LuaValueType_String: return m_valString < b.m_valString;
		case LuaValueType_Table: return m_valTable < b.m_valTable;
		}
		return false;
	}

	bool operator==(const LuaValue &b) const
	{
		if (m_type != b.m_type)
			return false;
		switch (m_type)
		{
		case LuaValueType_Nil: return true;
		case LuaValueType_Boolean: return m_valBool == b.m_valBool;
		case LuaValueType_Number: return m_valNumber == b.m_valNumber;
		case LuaValueType_String: return m_valString == b.m_valString;
		case LuaValueType_Table: return m_valTable == b.m_valTable;
		}
		return false;
	}

public:
	LuaValue() : m_type(LuaValueType_Nil), m_valBool(false), m_valNumber(0.0) {}

	bool isNil() const { return m_type == LuaValueType_Nil; }
	bool isBoolean() const { return m_type == LuaValueType_Boolean; }
	bool isNumber() const { return m_type == LuaValueType_Number; }
	bool isString() const { return m_type == LuaValueType_String; }
	bool isTable() const { return m_type == LuaValueType_Table; }

	string toString() const;
	void toString(ostringstream &os, uint32_t level=1) const;

	template <typename T>
	T as() const;

	const LuaValue *get(double fKey) const
	{
		LuaValue stKey;
		stKey.set(fKey);
		return get(stKey);
	}
	LuaValue *get(double fKey)
	{
		return const_cast<LuaValue *>(const_cast<const LuaValue *>(this)->get(fKey));
	}

	template <typename T>
	T get(double fKey) const
	{
		const LuaValue *val = get(fKey);
		if (val == NULL)
		{
			throw std::runtime_error("lua table miss number key: " + UtilString::tostr(fKey));
		}
		return val->as<T>();
	}

	const LuaValue *get(const string &sKey) const
	{
		LuaValue stKey;
		stKey.set(sKey);
		return get(stKey);
	}
	LuaValue *get(const string &sKey)
	{
		return const_cast<LuaValue *>(const_cast<const LuaValue *>(this)->get(sKey));
	}

	template <typename T>
	T get(const string &sKey) const
	{
		const LuaValue *val = get(sKey);
		if (val == NULL)
		{
			throw std::runtime_error("lua table miss string key: " + UtilString::tostr(sKey));
		}
		return val->as<T>();
	}

	void clear()
	{
		m_type = LuaValueType_Nil;
	}
	void set(bool v)
	{
		m_type = LuaValueType_Boolean;
		m_valBool = v;
	}
	void set(char v) { set((double)v); }
	void set(int8_t v) { set((double)v); }
	void set(uint8_t v) { set((double)v); }
	void set(int16_t v) { set((double)v); }
	void set(uint16_t v) { set((double)v); }
	void set(int32_t v) { set((double)v); }
	void set(uint32_t v) { set((double)v); }
	void set(int64_t v) { set((double)v); }
	void set(uint64_t v) { set((double)v); }
	void set(float v) { set((double)v); }
	void set(double v)
	{
		m_type = LuaValueType_Number;
		m_valNumber = v;
	}
	void set(const string &v)
	{
		m_type = LuaValueType_String;
		m_valString = v;
	}
	void set(const char *v)
	{
		if (v == NULL)
		{
			clear();
		}
		else
		{
			m_type = LuaValueType_String;
			m_valString = v;
		}
	}

	LuaValueProxy add(double fKey)
	{
		m_type = LuaValueType_Table;

		LuaValue stKey;
		stKey.set(fKey);

		LuaValue &stValue = m_valTable[stKey];
		LuaValueProxy prx;
		prx.m_parent = this;
		prx.m_data = &stValue;
		return prx;
	}

	LuaValueProxy add(const string &sKey)
	{
		m_type = LuaValueType_Table;

		LuaValue stKey;
		stKey.set(sKey);

		LuaValue &stValue = m_valTable[stKey];
		LuaValueProxy prx;
		prx.m_parent = this;
		prx.m_data = &stValue;
		return prx;
	}

	template <typename T>
	LuaValue &set(double fKey, const T &t)
	{
		add(fKey).set(t);
		return *this;
	}

	template <typename T>
	LuaValue &set(const string &sKey, const T &t)
	{
		add(sKey).set(t);
		return *this;
	}

private:
	const LuaValue *get(const LuaValue &stKey) const
	{
		if (!isTable())
		{
			throw std::runtime_error("lua table is required");
		}
		map<LuaValue, LuaValue>::const_iterator it = m_valTable.find(stKey);
		if (it != m_valTable.end())
		{
			return &it->second;
		}
		return NULL;
	}

private:
	friend class LuaModule;
	template <typename U> friend class impl::LuaValueCast;

	LuaValueType	m_type;
	bool 	m_valBool;
	double	m_valNumber;
	string	m_valString;
	map<LuaValue, LuaValue> m_valTable;
};

namespace impl
{

template <> struct LuaValueCast <bool>
{
	static bool cast(const LuaValue &stValue)
	{
		switch (stValue.m_type)
		{
		case LuaValue::LuaValueType_Nil: return false;
		case LuaValue::LuaValueType_Boolean: return stValue.m_valBool;
		case LuaValue::LuaValueType_Number: return stValue.m_valNumber != 0.0;
		case LuaValue::LuaValueType_String: return UtilString::strto<bool>(stValue.m_valString);
		case LuaValue::LuaValueType_Table: throw std::runtime_error("lua table cannot cast to boolean"); return false;
		}
		return false;
	}
};

template <> struct LuaValueCast <uint64_t>
{
	static uint64_t cast(const LuaValue &stValue)
	{
		switch (stValue.m_type)
		{
		case LuaValue::LuaValueType_Nil: return 0;
		case LuaValue::LuaValueType_Boolean: return stValue.m_valBool ? 1 : 0;
		case LuaValue::LuaValueType_Number: return (uint64_t)stValue.m_valNumber;
		case LuaValue::LuaValueType_String: return UtilString::strto<uint64_t>(stValue.m_valString);
		case LuaValue::LuaValueType_Table: throw std::runtime_error("lua table cannot cast to number"); return 0;
		}
		return 0;
	}
};

template<> struct LuaValueCast <double>
{
	static double cast(const LuaValue &stValue)
	{
		switch (stValue.m_type)
		{
		case LuaValue::LuaValueType_Nil: return 0.0;
		case LuaValue::LuaValueType_Boolean: return stValue.m_valBool ? 1.0 : 0.0;
		case LuaValue::LuaValueType_Number: return stValue.m_valNumber;
		case LuaValue::LuaValueType_String: return UtilString::strto<double>(stValue.m_valString);
		case LuaValue::LuaValueType_Table: throw std::runtime_error("lua table cannot cast to number"); return 0.0;
		}
		return 0.0;
	}
};

template<> struct LuaValueCast <string>
{
	static string cast(const LuaValue &stValue)
	{
		switch (stValue.m_type)
		{
		case LuaValue::LuaValueType_Nil: return "0";
		case LuaValue::LuaValueType_Boolean: return stValue.m_valBool ? "1" : "0";
		case LuaValue::LuaValueType_Number: return UtilString::tostr(stValue.m_valNumber);
		case LuaValue::LuaValueType_String: return stValue.m_valString;
		case LuaValue::LuaValueType_Table: throw std::runtime_error("lua table cannot cast to string"); return "";
		}
		return "";
	}
};

template <> struct LuaValueCast <int8_t>
{
	static int8_t cast(const LuaValue &stValue) { return static_cast<int8_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <uint8_t>
{
	static uint8_t cast(const LuaValue &stValue) { return static_cast<uint8_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <int16_t>
{
	static int16_t cast(const LuaValue &stValue) { return static_cast<int16_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <uint16_t>
{
	static uint16_t cast(const LuaValue &stValue) { return static_cast<uint16_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <int32_t>
{
	static int32_t cast(const LuaValue &stValue) { return static_cast<int32_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <uint32_t>
{
	static uint32_t cast(const LuaValue &stValue) { return static_cast<uint32_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <int64_t>
{
	static int64_t cast(const LuaValue &stValue) { return static_cast<int64_t>(LuaValueCast<uint64_t>::cast(stValue)); }
};

template<> struct LuaValueCast <float>
{
	static float cast(const LuaValue &stValue) { return static_cast<float>(LuaValueCast<double>::cast(stValue)); }
};

}

template <typename T>
T LuaValue::as() const
{
	return impl::LuaValueCast<T>::cast(*this);
}

class LuaModule
{
public:
	LuaModule();
	LuaModule(lua_State *LL, bool bOwned);
	~LuaModule();

	void setOwned(bool bOwned) { m_bOwned = bOwned; }
	lua_State *getState() { return L; }

	void addPath(const string &sPath);
	void setPath(const string &sPath);
	void addCPath(const string &sCPath);
	void setCPath(const string &sCPath);

	int gettop();
	void settop(int idx);
	void pop(int n);
	string tostring(int idx);
	bool toboolean(int idx);
	int32_t tointeger(int idx);
	double tonumber(int idx);
    void createtable(int narr = 0, int nrec = 0);

	int pcall(int nargs, int nresults = LUA_MULTRET, int errfunc = 0);
	bool doFile(const string &sFileName, int nresults = 0);
	bool doString(const string &sScript, int nresults = 0);

	void push(bool v);
	void push(char v) { push((uint32_t)v); }
	void push(int8_t v) { push((int32_t)v); }
	void push(uint8_t v) { push((uint32_t)v); }
	void push(int16_t v) { push((int32_t)v); }
	void push(uint16_t v) { push((uint32_t)v); }
	void push(int32_t v);
	void push(uint32_t v);
	void push(int64_t v);
	void push(uint64_t v);
	void push(float v) { push((double)v); }
	void push(double v);
	void push(const string &v);
    void push(const char *v);
    void push(const LuaValue &v);

    template <typename T, typename Alloc>
    void push(const vector<T, Alloc> &v)
    {
        createtable(0, 0);
        for (unsigned i = 0; i < v.size(); ++i)
        {
            push(v[i]);
            lua_rawseti(L, -2, i + 1);
        }
    }

    template <typename Key, typename Value, typename Compare, typename Alloc>
    void push(const map<Key, Value, Compare, Alloc> &v)
    {
        createtable(0, 0);
        for (typename map<Key, Value, Compare, Alloc>::const_iterator first = v.begin(), last = v.end(); first != last; ++first)
        {
            push(first->first);
            push(first->second);
            lua_rawset(L, -3);
        }
    }

	template <typename T>
	void push(const T &v)
	{
		push(sdpToString(v));
	}

	void peek(int idx, bool &v) { v = toboolean(idx); }
	void peek(int idx, char &v) { v = (char)tointeger(idx); }
	void peek(int idx, int8_t &v) { v = (int8_t)tointeger(idx); }
	void peek(int idx, uint8_t &v) { v = (uint8_t)tointeger(idx); }
	void peek(int idx, int16_t &v) { v = (int16_t)tointeger(idx); }
	void peek(int idx, uint16_t &v) { v = (uint16_t)tointeger(idx); }
	void peek(int idx, int32_t &v) { v = tointeger(idx); }
	void peek(int idx, uint32_t &v) { v = (uint32_t)tonumber(idx); }
	void peek(int idx, int64_t &v);
	void peek(int idx, uint64_t &v);
	void peek(int idx, float &v) {v = (float)tonumber(idx); }
	void peek(int idx, double &v) { v = tonumber(idx); }
	void peek(int idx, string &v) { v = tostring(idx); }
    void peek(int idx, LuaValue &v);

    template <typename T, typename Alloc>
    void peek(int idx, vector<T, Alloc> &v)
    {
        int type = lua_type(L, idx);
        if (type == LUA_TNIL)
        {
            return;
        }
        if (type != LUA_TTABLE)
        {
            throw std::runtime_error("vector table expect but got " + string(lua_typename(L, type)));
        }

        unsigned iSize = lua_objlen(L, idx);
        for (unsigned i = 0; i < iSize; ++i)
        {
            KEEP_LUA_STACK();
            lua_rawgeti(L, idx, i + 1);

            T t;
            peek(-1, t);
            v.push_back(t);
        }
    }

    template <typename Key, typename Value, typename Compare, typename Alloc>
    void peek(int idx, map<Key, Value, Compare, Alloc> &v)
    {
        int type = lua_type(L, idx);
        if (type == LUA_TNIL)
        {
            return;
        }
        if (type != LUA_TTABLE)
        {
            throw std::runtime_error("map table expect but got " + string(lua_typename(L, type)));
        }

        KEEP_LUA_STACK();
        lua_pushnil(L);
        --idx;
        while (lua_next(L, idx) != 0)
        {
            Key key;
            peek(-2, key);
            peek(-1, v[key]);

            lua_pop(L, 1);
        }
    }
	
	template <typename T>
	void peek(int idx, T &v)
	{
		string s = tostring(idx);
		if (!s.empty())
		{
			stringToSdp(s, v);
		}
	}

	template <typename P1, typename P2>
	void push(const P1 &p1, const P2 &p2)
	{ push(p1); push(p2); }

	template <typename P1, typename P2, typename P3>
	void push(const P1 &p1, const P2 &p2, const P3 &p3)
	{ push(p1); push(p2); push(p3); }

	template <typename P1, typename P2, typename P3, typename P4>
	void push(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4)
	{ push(p1); push(p2); push(p3); push(p4); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	void push(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5)
	{ push(p1); push(p2); push(p3); push(p4); push(p5); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	void push(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6)
	{ push(p1); push(p2); push(p3); push(p4); push(p5); push(p6); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	void push(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7)
	{ push(p1); push(p2); push(p3); push(p4); push(p5); push(p6); push(p7); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
	void push(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, const P8 &p8)
	{ push(p1); push(p2); push(p3); push(p4); push(p5); push(p6); push(p7); push(p8); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
	void push(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, const P8 &p8, const P9 &p9)
	{ push(p1); push(p2); push(p3); push(p4); push(p5); push(p6); push(p7); push(p8); push(p9); }

	template <typename P1, typename P2>
	void peek(int idx, P1 &p1, P2 &p2)
	{ idx -= 2; peek(++idx, p1); peek(++idx, p2); }

	template <typename P1, typename P2, typename P3>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3)
	{ idx -= 3; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); }

	template <typename P1, typename P2, typename P3, typename P4>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3, P4 &p4)
	{ idx -= 4; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); peek(++idx, p4); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5)
	{ idx -= 5; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); peek(++idx, p4); peek(++idx, p5); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6)
	{ idx -= 6; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); peek(++idx, p4); peek(++idx, p5); peek(++idx, p6); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6, P7 &p7)
	{ idx -= 7; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); peek(++idx, p4); peek(++idx, p5); peek(++idx, p6); peek(++idx, p7); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6, P7 &p7, P8 &p8)
	{ idx -= 8; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); peek(++idx, p4); peek(++idx, p5); peek(++idx, p6); peek(++idx, p7); peek(++idx, p8); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
	void peek(int idx, P1 &p1, P2 &p2, P3 &p3, P4 &p4, P5 &p5, P6 &p6, P7 &p7, P8 &p8, P9 &p9)
	{ idx -= 9; peek(++idx, p1); peek(++idx, p2); peek(++idx, p3); peek(++idx, p4); peek(++idx, p5); peek(++idx, p6); peek(++idx, p7); peek(++idx, p8); peek(++idx, p9); }

	void push(const tr1::tuple<> &/*tpIn*/)
	{ }

	template <typename P1>
	void push(const tr1::tuple<P1> &tpIn)
	{ push(tr1::get<0>(tpIn)); }

	template <typename P1, typename P2>
	void push(const tr1::tuple<P1, P2> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn)); }

	template <typename P1, typename P2, typename P3>
	void push(const tr1::tuple<P1, P2, P3> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn)); }

	template <typename P1, typename P2, typename P3, typename P4>
	void push(const tr1::tuple<P1, P2, P3, P4> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn), tr1::get<3>(tpIn)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	void push(const tr1::tuple<P1, P2, P3, P4, P5> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn), tr1::get<3>(tpIn), tr1::get<4>(tpIn)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	void push(const tr1::tuple<P1, P2, P3, P4, P5, P6> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn), tr1::get<3>(tpIn), tr1::get<4>(tpIn), tr1::get<5>(tpIn)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	void push(const tr1::tuple<P1, P2, P3, P4, P5, P6, P7> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn), tr1::get<3>(tpIn), tr1::get<4>(tpIn), tr1::get<5>(tpIn), tr1::get<6>(tpIn)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
	void push(const tr1::tuple<P1, P2, P3, P4, P5, P6, P7, P8> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn), tr1::get<3>(tpIn), tr1::get<4>(tpIn), tr1::get<5>(tpIn), tr1::get<6>(tpIn), tr1::get<7>(tpIn)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
	void push(const tr1::tuple<P1, P2, P3, P4, P5, P6, P7, P8, P9> &tpIn)
	{ push(tr1::get<0>(tpIn), tr1::get<1>(tpIn), tr1::get<2>(tpIn), tr1::get<3>(tpIn), tr1::get<4>(tpIn), tr1::get<5>(tpIn), tr1::get<6>(tpIn), tr1::get<7>(tpIn), tr1::get<8>(tpIn)); }

	void peek(int /*idx*/, tr1::tuple<> &/*tpOut*/)
	{ }

	template <typename P1>
	void peek(int idx, tr1::tuple<P1> &tpOut)
	{ peek(idx, tr1::get<0>(tpOut)); }

	template <typename P1, typename P2>
	void peek(int idx, tr1::tuple<P1, P2> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut)); }

	template <typename P1, typename P2, typename P3>
	void peek(int idx, tr1::tuple<P1, P2, P3> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut)); }

	template <typename P1, typename P2, typename P3, typename P4>
	void peek(int idx, tr1::tuple<P1, P2, P3, P4> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut), tr1::get<3>(tpOut)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	void peek(int idx, tr1::tuple<P1, P2, P3, P4, P5> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut), tr1::get<3>(tpOut), tr1::get<4>(tpOut)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	void peek(int idx, tr1::tuple<P1, P2, P3, P4, P5, P6> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut), tr1::get<3>(tpOut), tr1::get<4>(tpOut), tr1::get<5>(tpOut)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	void peek(int idx, tr1::tuple<P1, P2, P3, P4, P5, P6, P7> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut), tr1::get<3>(tpOut), tr1::get<4>(tpOut), tr1::get<5>(tpOut), tr1::get<6>(tpOut)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
	void peek(int idx, tr1::tuple<P1, P2, P3, P4, P5, P6, P7, P8> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut), tr1::get<3>(tpOut), tr1::get<4>(tpOut), tr1::get<5>(tpOut), tr1::get<6>(tpOut), tr1::get<7>(tpOut)); }

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
	void peek(int idx, tr1::tuple<P1, P2, P3, P4, P5, P6, P7, P8, P9> tpOut)
	{ peek(idx, tr1::get<0>(tpOut), tr1::get<1>(tpOut), tr1::get<2>(tpOut), tr1::get<3>(tpOut), tr1::get<4>(tpOut), tr1::get<5>(tpOut), tr1::get<6>(tpOut), tr1::get<7>(tpOut), tr1::get<8>(tpOut)); }

	template <typename InputTuple, typename OutputTuple>
	bool doCall(const char *func, const InputTuple &tpIn, OutputTuple tpOut)
	{
		KEEP_LUA_STACK();
		lua_pushcfunction(L, traceback);
		int errfunc = gettop();
		lua_getglobal(L, func);
		push(tpIn);

		int nargs = tr1::tuple_size<InputTuple>::value;
		int nresults = tr1::tuple_size<OutputTuple>::value;
		if (pcall(nargs, nresults, errfunc) != 0)
		{
			throw std::runtime_error(tostring(-1));
		}
		peek(-1, tpOut);
		return true;
	}

	template <typename InputTuple>
	bool doCall(const char *func, const InputTuple &tpIn)
	{
		return doCall(func, tpIn, tr1::tuple<>());
	}

	bool doCallVoid(const char *func)
	{
		return doCall(func, tr1::tuple<>());
	}

	template <typename P1>
	bool doCallVoid(const char *func, const P1 &p1)
	{
		return doCall(func, tr1::tie(p1));
	}

	template <typename P1, typename P2>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2)
	{
		return doCall(func, tr1::tie(p1, p2));
	}

	template <typename P1, typename P2, typename P3>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3)
	{
		return doCall(func, tr1::tie(p1, p2, p3));
	}

	template <typename P1, typename P2, typename P3, typename P4>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4));
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5));
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6));
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6, p7));
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, const P8 &p8)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6, p7, p8));
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9>
	bool doCallVoid(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, const P8 &p8, const P9 &p9)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9));
	}

	template <typename OutputTuple>
	bool doCallRet(const char *func, OutputTuple tpOut)
	{
		return doCall(func, tr1::tuple<>(), tpOut);
	}

	template <typename P1, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1), tpOut);
	}

	template <typename P1, typename P2, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename P4, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6, p7), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, const P8 &p8, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6, p7, p8), tpOut);
	}

	template <typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7, typename P8, typename P9, typename OutputTuple>
	bool doCallRet(const char *func, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5, const P6 &p6, const P7 &p7, const P8 &p8, const P9 &p9, OutputTuple tpOut)
	{
		return doCall(func, tr1::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9), tpOut);
	}

	void registerlib(const char *libname, const luaL_Reg *reg);

	// 注册mfwlog.debug/info/error/daylog/hourlog
	void registerMfwLog(const char *libname = "mfwlog");

protected:
	void modifyPackagePath(const string &sPath, const char *key, bool bAdd);
	static int traceback(lua_State *L);

protected:
	lua_State *L;
	bool m_bOwned;
};

}
#endif
