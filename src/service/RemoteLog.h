#ifndef _MFW_REMOTELOG_H_
#define _MFW_REMOTELOG_H_

#include <string>
using namespace std;

namespace mfw
{

class MfwRemoteLog
{
public:
	static void initRemoteLog(const string &sLogObj, const string &sGlobalLogObj);
};

}
#endif
