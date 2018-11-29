#include "service/ServiceCreator.h"

namespace mfw
{

ServicePtr ServiceCreatorManager::create(const string &sServiceName)
{
	ServicePtr service;
	map<string, ServiceCreatorPtr>::iterator it = m_mServiceCreator.find(sServiceName);
    if (it != m_mServiceCreator.end())
    {
    	ServiceCreatorPtr &creator = it->second;
        service = creator->create(sServiceName);
    }
    return service;
}

}


