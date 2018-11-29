#include "util/util_time.h"
#include "service/ServiceHandle.h"
#include "service/Application.h"
#include "service/ServiceCreator.h"
#include "service/MfwProtocol.h"
#include "service/ReturnCode.h"
#include "service/Connector.h"
#include <stdlib.h>

namespace mfw
{

ServiceHandle::~ServiceHandle()
{
	if (m_service)
	{
		m_service->destroy();
	}
}

bool ServiceHandle::initialize()
{
	m_service = ServiceCreatorManager::getInstance()->create(m_pBindAdapter->getServiceName());
	if (!m_service)
	{
		MFW_ERROR("fail to create service: " << m_pBindAdapter->getServiceName());
        return false;
	}

	try
	{
		m_service->initialize();
	}
	catch (std::exception &e)
	{
		MFW_ERROR("service initialize exception: " << e.what());
        return false;
	}

    return true;
}

SdpCurrentPtr ServiceHandle::createCurrent(const tagRecvData &stRecvData)
{
    SdpCurrentPtr current(new SdpCurrent(this));

    try
    {
        current->initialize(stRecvData);
    }
    catch (std::exception &e)
    {
        MFW_ERROR("request packect error: " << e.what());
        closeConnection(stRecvData.uid);
        return SdpCurrentPtr();
    }

    if (current->getBindAdapter()->isMfwProtocol())
    {
        uint64_t iNowMS = UtilTime::getNowMS();
        if (current->getRequestTimeout() > 0 && iNowMS > stRecvData.recvTimeStamp + current->getRequestTimeout())
        {
        	MFW_ERROR("server queue timeout, peer: " << current->getIp() << ":" << current->getPort() << ", obj: " << current->getServiceName() << ", func: " << current->getFuncName());
            current->sendMfwResponse(SDPSERVERQUEUETIMEOUT);
            return SdpCurrentPtr();
        }
    }
    return current;
}

SdpCurrentPtr ServiceHandle::createCloseCurrent(const tagRecvData &stRecvData)
{
	SdpCurrentPtr current(new SdpCurrent(this));
    current->initializeClose(stRecvData);
	return current;
}

void ServiceHandle::handleClose(const tagRecvData &stRecvData)
{
	SdpCurrentPtr current = createCloseCurrent(stRecvData);
	MFW_DEBUG("handle close, peer: " << current->getIp() << ":" << current->getPort() << ", obj: " << current->getServiceName());

	m_service->setCurrent(current);
    try
    {
    	m_service->handleClose();
    }
    catch (std::exception &e)
    {
        MFW_ERROR("dispatch close event exception: " << e.what());
    }
    m_service->unsetCurrent();
}

void ServiceHandle::handleTimeout(const tagRecvData &stRecvData)
{
    SdpCurrentPtr current = createCurrent(stRecvData);
    if (!current)
    {
    	return;
    }

    if (current->getBindAdapter()->isMfwProtocol())
    {
    	MFW_ERROR("handle timeout, peer: " << current->getIp() << ":" << current->getPort()
    			<< ", obj: " << current->getServiceName() << ", func: " << current->getFuncName() << ", reqid: " << current->getRequestId());
        current->sendMfwResponse(SDPSERVERQUEUETIMEOUT);
    }
    else
    {
    	MFW_ERROR("handle timeout, peer: " << current->getIp() << ":" << current->getPort()
    			<< ", obj: " << current->getServiceName());
    }
}

void ServiceHandle::handleOverload(const tagRecvData &stRecvData)
{
    SdpCurrentPtr current = createCurrent(stRecvData);
    if (!current)
    {
    	return;
    }

    if (current->getBindAdapter()->isMfwProtocol())
    {
    	MFW_ERROR("handle overload, peer: " << current->getIp() << ":" << current->getPort()
    			<< ", obj: " << current->getServiceName() << ", func: " << current->getFuncName() << ", reqid: " << current->getRequestId());
        current->sendMfwResponse(SDPSERVEROVERLOAD);
    }
    else
    {
    	MFW_ERROR("handle overload, peer: " << current->getIp() << ":" << current->getPort()
    			<< ", obj: " << current->getServiceName());
    }
}

void ServiceHandle::handle(const tagRecvData &stRecvData)
{
    SdpCurrentPtr current = createCurrent(stRecvData);
    if (!current)
    {
    	return;
    }

    if (current->getBindAdapter()->isMfwProtocol())
    {
        handleMfwProtocol(current);
    }
    else
    {
        handleNoMfwProtocol(current);
    }
}

void ServiceHandle::handleMfwProtocol(const SdpCurrentPtr &current)
{
	MFW_DEBUG("handle mfw request, peer: " << current->getIp() << ":" << current->getPort()
			<< ", obj: " << current->getServiceName() << ", func: " << current->getFuncName() << ", reqid: " << current->getRequestId());

    if (m_service->getServiceName() != current->getServiceName())
    {
        current->sendMfwResponse(SDPSERVERNOSERVICEERR);
        return;
    }

    int32_t ret = SDPSERVERUNKNOWNERR;
    string sRspPayload;
    m_service->setCurrent(current);
    try
    {
		ret = m_service->handleMfwRequest(sRspPayload);
    }
    catch (std::exception &e)
    {
        MFW_ERROR("dispatch mfw msg exception: " << e.what());
    }
    m_service->unsetCurrent();

    if (current->isResponse())
    {
        current->sendMfwResponse(ret, sRspPayload);
    }
}

void ServiceHandle::handleNoMfwProtocol(const SdpCurrentPtr &current)
{
	MFW_DEBUG("handle nomfw request, peer: " << current->getIp() << ":" << current->getPort() << ", obj: " << current->getServiceName());

    string sRspPayload;
    m_service->setCurrent(current);
    try
    {
    	m_service->handleNoMfwRequest(current->getRequestBuffer(), sRspPayload);
    }
    catch (std::exception &e)
    {
        MFW_ERROR("dispatch nomfw msg exception: " << e.what());
    }
    m_service->unsetCurrent();

    if (current->isResponse())
    {
        current->sendNoMfwResponse(sRspPayload);
    }
}

}
