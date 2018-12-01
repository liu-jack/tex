#ifndef _MFW_UTIL_OPTION_
#define _MFW_UTIL_OPTION_

#include <map>
#include <vector>
#include <string>
using namespace std;

namespace mfw
{

class COption
{
public:
    void parse(int argc, char *argv[]);
    bool hasOption(const string &sName) const;
    string getOption(const string &sName) const;
    string getOption(const string &sName, const string &sDefault) const;
    const map<string, string> &getOption() const
    {
        return m_mOption;
    }
    const vector<string> &getParam() const
    {
        return m_vParam;
    }

private:
    map<string, string> m_mOption;
    vector<string>      m_vParam;
};

}

#endif

