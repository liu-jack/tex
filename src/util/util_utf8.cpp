#include "util_utf8.h"
#include "utf8/utf8.h"

namespace mfw
{

bool UtilUTF8::isValidUTF8(const string &s)
{
	return utf8::is_valid(s.begin(), s.end());
}

uint32_t UtilUTF8::utf8Length(const string &s)
{
	return utf8::distance(s.begin(), s.end());
}

string UtilUTF8::utf8UpperCase(const string &s)
{
	string sResult;
	string::const_iterator start = s.begin();
	string::const_iterator end = s.end();
	for ( ; start != end; )
	{
		string::const_iterator it = start;
		uint32_t cp = utf8::next(start, end);
		if (cp <= 255)
		{
			sResult.append(1, (char)toupper(cp));
		}
		else
		{
			sResult.append(it, start);
		}
	}
	return sResult;
}

string UtilUTF8::utf8LowerCase(const string &s)
{
	string sResult;
	string::const_iterator start = s.begin();
	string::const_iterator end = s.end();
	for ( ; start != end; )
	{
		string::const_iterator it = start;
		uint32_t cp = utf8::next(start, end);
		if (cp <= 255)
		{
			sResult.append(1, (char)tolower(cp));
		}
		else
		{
			sResult.append(it, start);
		}
	}
	return sResult;
}

string UtilUTF8::utf8Substr(const string &s, uint32_t pos, uint32_t len)
{
	string sResult;
	string::const_iterator start = s.begin();
	string::const_iterator end = s.end();
	uint32_t endpos = pos + len;
	for (uint32_t i = 0; start != end && i < endpos; ++i)
	{
		string::const_iterator it = start;
		utf8::next(start, end);
		if(i >= pos)
		{
			sResult.append(it, start);
		}
	}
	return sResult;
}

}
