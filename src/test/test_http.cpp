#include "util/util_string.h"
#include "util/util_http.h"
#include <iostream>

using namespace std;
using namespace mfw;

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		cout << "Usage:" << argv[0] << " url" << endl;
		return -1;
	}

	UtilHttp::ResponseData stResponse;
	UtilHttp::RequestOption stRequestOption;
	stRequestOption.bVerbose = true;

	UtilHttp::fetchURL(argv[1], stResponse, stRequestOption);

	cout << stResponse.iCode << endl;
	map<string, string>::const_iterator iter = stResponse.mHeader.begin();
	while (iter != stResponse.mHeader.end())
	{
		cout << iter->first << ":" << iter->second << endl;
		++iter;
	}
	cout << stResponse.sResponse << endl;	

	return 0;
}
