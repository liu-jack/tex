#ifndef _MFW_TRANSPORT_H_
#define _MFW_TRANSPORT_H_

#include "util/util_network.h"
#include "service/MfwPacket.h"
#include "service/Global.h"
#include <list>

using namespace std;

namespace mfw
{
class AdapterProxy;

class Transport
{
public:
    enum ConnectStatus {
        eUnconnected,
        eConnecting,
        eConnected,
    };

    explicit Transport(AdapterProxy *pAdapterProxy);
    virtual ~Transport();

    bool isValid() const
    {
        return m_sock.isValid();
    }
    CSocket &getSocket()
    {
        return m_sock;
    }
    AdapterProxy *getAdapterProxy()
    {
        return m_pAdapterProxy;
    }

    bool isConnecting()
    {
        return isValid() && (m_connStatus == eConnecting);
    }
    bool isConnected()
    {
        return isValid() && (m_connStatus == eConnected);
    }
    void setConnected();

    void checkTimeout();
    void reconnect();
    void connect();
    void close();

public:
    virtual int doRequest();
    virtual int doResponse(list<ResponsePacket> &done) = 0;
    bool trySendRequest(const char *buf, uint32_t len);

    virtual int send(const void *buf, uint32_t len) = 0;
    virtual int recv(void *buf, uint32_t len) = 0;

protected:
    AdapterProxy *m_pAdapterProxy;
    CEndpoint m_ep;
    CSocket m_sock;
    ConnectStatus m_connStatus;
    uint64_t m_iConnectTimeoutTime;

    string m_sSendBuffer;
    string m_sRecvBuffer;
};

class TcpTransport : public Transport
{
public:
    explicit TcpTransport(AdapterProxy *pAdapterProxy) : Transport(pAdapterProxy) {}

    virtual int send(const void *buf, uint32_t len);
    virtual int recv(void *buf, uint32_t len);
    virtual int doResponse(list<ResponsePacket> &done);
};

class UdpTransport : public Transport
{
public:
    explicit UdpTransport(AdapterProxy *pAdapterProxy) : Transport(pAdapterProxy) {}

    virtual int send(const void *buf, uint32_t len);
    virtual int recv(void *buf, uint32_t len);
    virtual int doResponse(list<ResponsePacket> &done);
};

}
#endif
