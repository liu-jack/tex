#ifndef _MFW_SERVICE_CREATOR_H_
#define _MFW_SERVICE_CREATOR_H_

#include <map>
#include "util/util_singleton.h"
#include "service/Service.h"

namespace mfw
{

class ServiceCreatorManager : public CSingleton<ServiceCreatorManager>
{
    class ServiceCreator
    {
    public:
        virtual ~ServiceCreator() {}
        virtual ServicePtr create(const string &sServiceName) = 0;
    };
    typedef tr1::shared_ptr<ServiceCreator> ServiceCreatorPtr;

    template<class T>
    struct ServiceCreatorConcrete : public ServiceCreator {
        ServicePtr create(const string &sServiceName)
        {
            ServicePtr p(new T);
            p->setServiceName(sServiceName);
            return p;
        }
    };

public:
    template<typename T>
    void addService(const string &sServiceName)
    {
        m_mServiceCreator[sServiceName].reset(new ServiceCreatorConcrete<T>());
    }

    ServicePtr create(const string &sServiceName);

private:
    map<string, ServiceCreatorPtr> m_mServiceCreator;
};

}

#endif

