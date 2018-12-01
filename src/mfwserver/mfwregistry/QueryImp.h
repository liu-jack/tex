#ifndef __QUERY_IMP_H__
#define __QUERY_IMP_H__

#include "registry/Query.h"
using namespace mfw;

class QueryImp: public Query
{
public:
    QueryImp() {}
    virtual ~QueryImp() {}

    virtual void initialize() {}
    virtual void destroy() {}

    virtual int32_t getEndpoints(const string &sObj, const string &sDivision, vector<string> &vActiveEps, vector<string> &vInactiveEps);
    virtual int32_t addEndpoint(const string &sObj, const string &sDivision, const string &sEp);
    virtual int32_t removeEndpoint(const string &sObj, const string &sDivision, const string &sEp);
};

#endif
