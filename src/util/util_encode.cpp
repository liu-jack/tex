#include "util_encode.h"
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace mfw
{


static const char g_hexcode[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

string UtilEncode::toHex(const string &s)
{
	string r;
	r.resize(s.size() * 2);
	for (string::size_type i = 0; i < s.size(); ++i)
	{
		uint8_t c = s[i];
		r[i * 2] = g_hexcode[c >> 4];
		r[i * 2 + 1] = g_hexcode[c & 0xf];
	}
	return r;
}

string UtilEncode::fromHex(const string &s)
{
	string r;
	string::size_type n = s.size() / 2;
	r.resize(n);
	for (string::size_type i = 0; i < n; ++i)
	{
		uint8_t hi = s[i * 2];
		uint8_t lo = s[i * 2 + 1];
		hi = hi >= 'a' ? (hi - 'a' + 10) : (hi >= 'A' ? (hi - 'A' + 10) : hi - '0');
		lo = lo >= 'a' ? (lo - 'a' + 10) : (lo >= 'A' ? (lo - 'A' + 10) : lo - '0');
		r[i] = (hi << 4) | lo;
	}
	return r;
}

string UtilEncode::c_unescape(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		if (s[i] != '\\' || i + 1 == s.size())
		{
			r.push_back(s[i]);
			continue;
		}

		char c = s[++i];
		switch(c)
		{
		case 'a': r.push_back('\a'); break;
		case 'b': r.push_back('\b'); break;
		case 'f': r.push_back('\f'); break;
		case 'n': r.push_back('\n'); break;
		case 'r': r.push_back('\r'); break;
		case 't': r.push_back('\t'); break;
		case 'v': r.push_back('\v'); break;
		case '\\': r.push_back('\\'); break;
		case '\'': r.push_back('\''); break;
		case '"': r.push_back('\"'); break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
			{
				unsigned char val = c - '0';
				if (i + 1 < s.size() && s[i + 1] >= '0' && s[i + 1] <= '7')
				{
					c = s[++i];
					val = val * 8 + (c - '0');
					if (val <= 037 && i + 1 < s.size() && s[i + 1] >= '0' && s[i + 1] <= '7')
					{
						c = s[++i];
						val = val * 8 + (c - '0');
					}
				}
				r.push_back(val);
			}
			break;
		case 'x':
			{
				if (i + 1 < s.size() && ::isxdigit(s[i + 1]))
				{
					c = ::tolower(s[++i]);
					unsigned char val = c >= 'a' ? (c - 'a' + 10) : (c - '0');
					if (i + 1 < s.size() && ::isxdigit(s[i + 1]))
					{
						c = ::tolower(s[++i]);
						val = val * 16 + (c >= 'a' ? (c - 'a' + 10) : (c - '0'));
					}
					r.push_back(val);
				}
				else
				{
					r.push_back('\\');
					r.push_back(c);
				}
			}
			break;
		default:
			r.push_back(c);
			break;
		}
	}
	return r;
}

string UtilEncode::c_escape(const string &s)
{
	string r;
	for (unsigned i = 0; i < s.size(); ++i)
	{
		char c = s[i];
		switch(c)
		{
		case '\a': r.append("\\a"); break;
		case '\b': r.append("\\b"); break;
		case '\f': r.append("\\f"); break;
		case '\n': r.append("\\n"); break;
		case '\r': r.append("\\r"); break;
		case '\t': r.append("\\t"); break;
		case '\v': r.append("\\v"); break;
		case '\\': r.append("\\\\"); break;
		case '\'': r.append("\\\'"); break;
		case '"': r.append("\\\""); break;
		default:
			{
				if (isprint(c))
				{
					r.push_back(c);
				}
				else
				{
					r.append("\\x");
					char buf[4];
					snprintf(buf, sizeof(buf), "%02x", c);
					r.append(buf);
				}
			}
			break;
		}
	}
	return r;
}

static bool isunreserved(unsigned char in)
{
    switch (in)
    {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '-': case '.': case '_':
            return true;
        default:
            break;
    }
    return false;
}

string UtilEncode::urlencode(const string &sUrl)
{
    const char *p = sUrl.c_str();
    unsigned char in;

    ostringstream os;

    while (*p) 
    {   
        in = *p;                
        if (isunreserved(in))
        {   
            os << in; 
        } 
        else if (in == ' ')
        {
            os << '+';
        }
        else
        {   
            os << "%" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)in;
        }   
        ++p;
    }       

    return os.str();
}               

#define ISXDIGIT(x) (isxdigit((int) ((unsigned char)x)))

string UtilEncode::urldecode(const string &sUrl)
{   
    const char *p = sUrl.c_str();
    const char *e = p+sUrl.size()-1;
    unsigned char in;
    char hex[3];
    unsigned char iHex;

    ostringstream os;

    while (*p)
    {
        in = *p;
        if (in == '%' && (e-p>=2) && ISXDIGIT(*(p+1)) && ISXDIGIT(*(p+2)))
        {
            hex[0]=*(p+1);
            hex[1]=*(p+2);
            hex[2]=0;
            iHex = strtoul(hex, NULL, 16);
            in = static_cast<unsigned char>(iHex);
            p += 2;
        }
        else if (in == '+')
        {
            in = ' ';
        }
        os << in;
        ++p;
    }

    return os.str();
}       

string UtilEncode::rawurlencode(const string &sUrl)
{
    const char *p = sUrl.c_str();
    unsigned char in;

    ostringstream os;

    while (*p) 
    {   
        in = *p;                
        if (isunreserved(in))
        {   
            os << in; 
        } 
        else if (in == '~')
        {
            os << in;
        }
        else
        {   
            os << "%" << std::hex << std::setw(2) << std::setfill('0') << (uint32_t)in;
        }   
        ++p;
    }       

    return os.str();
}               

#define ISXDIGIT(x) (isxdigit((int) ((unsigned char)x)))

string UtilEncode::rawurldecode(const string &sUrl)
{   
    const char *p = sUrl.c_str();
    const char *e = p+sUrl.size()-1;
    unsigned char in;
    char hex[3];
    unsigned char iHex;

    ostringstream os;

    while (*p)
    {
        in = *p;
        if (in == '%' && (e-p>=2) && ISXDIGIT(*(p+1)) && ISXDIGIT(*(p+2)))
        {
            hex[0]=*(p+1);
            hex[1]=*(p+2);
            hex[2]=0;
            iHex = strtoul(hex, NULL, 16);
            in = static_cast<unsigned char>(iHex);
            p += 2;
        }
        os << in;
        ++p;
    }

    return os.str();
}

}
