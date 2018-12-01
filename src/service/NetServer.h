#ifndef _MFW_NET_SERVER_H_
#define _MFW_NET_SERVER_H_

#include "util/util_time.h"
#include "util/util_thread.h"
#include "util/util_queue.h"
#include "util/util_network.h"
#include <map>

namespace mfw
{

class NetServer;
typedef tr1::shared_ptr<NetServer> NetServerPtr;

class BindAdapter;
typedef tr1::shared_ptr<BindAdapter> BindAdapterPtr;

class Handle;
typedef tr1::shared_ptr<Handle> HandlePtr;

class NetThread;

typedef int (*protocol_functor)(const char *pInBegin, const char *pInEnd, const char *&pNextIn, string &sOut);

struct tagRecvData {
    uint32_t        uid;
    string          buffer;
    string          ip;
    uint16_t        port;
    uint64_t        recvTimeStamp;
    bool            isOverload;
    bool            isClosed;
    BindAdapter     *adapter;
};

struct tagSendData {
    uint32_t        uid;
    char            cmd;
    string          buffer;
    string          ip;
    uint16_t        port;
};

class Handle : public CThreadBase
{
public:
    Handle() : m_pBindAdapter(NULL) {}
    virtual ~Handle() {}

    void setBindAdapter(BindAdapter *pBindAdapter)
    {
        m_pBindAdapter = pBindAdapter;
    }
    void sendResponse(uint32_t uid, const string &sData, const string &sIp, uint16_t iPort);
    void closeConnection(uint32_t uid);

    virtual void run();

protected:
    virtual bool initialize()
    {
        return false;
    }
    virtual void handle(const tagRecvData &stRecvData) = 0;
    virtual void handleTimeout(const tagRecvData &stRecvData) = 0;
    virtual void handleOverload(const tagRecvData &stRecvData) = 0;
    virtual void handleClose(const tagRecvData &stRecvData) = 0;

protected:
    BindAdapter *m_pBindAdapter;
};

class BindAdapter
{
public:
    BindAdapter(NetServer *pNetServer, const string &sServiceName);

    const string &getServiceName() const
    {
        return m_sServiceName;
    }
    NetServer *getNetServer()
    {
        return m_pNetServer;
    }

    void setEndpoint(const string &sEndpoint)
    {
        m_listenEndpoint.parse(sEndpoint);
    }
    const CEndpoint &getEndpoint() const
    {
        return m_listenEndpoint;
    }
    void setMaxConns(int iMaxConns)
    {
        m_iMaxConns = iMaxConns;
    }
    uint32_t getMaxConns() const
    {
        return m_iMaxConns;
    }
    void setQueueCapacity(uint32_t iCapacity)
    {
        m_iQueueCapacity = iCapacity;
    }
    uint32_t getQueueCapacity() const
    {
        return m_iQueueCapacity;
    }
    void setQueueTimeout(uint32_t iTimeout)
    {
        m_iQueueTimeout = iTimeout;
    }
    uint32_t getQueueTimeout() const
    {
        return m_iQueueTimeout;
    }
    void setProtocolName(const string &sName)
    {
        m_sProtocolName = sName;
        m_bIsMfwProtocol = m_sProtocolName == "mfw";
    }
    const string &getProtocolName()
    {
        return m_sProtocolName;
    }
    bool isMfwProtocol()
    {
        return m_bIsMfwProtocol;
    }
    void setProtocol(protocol_functor fn)
    {
        m_fnProtocol = fn;
    }
    protocol_functor getProtocol()
    {
        return m_fnProtocol;
    }
    CSocket &getSocket()
    {
        return m_servsock;
    }

    void insertRecvQueue(tagRecvData *pstRecvData, bool bCheckOverload);
    void wakeupRecvQueue();
    bool waitForRecvQueue(tagRecvData* &pstRecvData, int64_t iWaitTime);

    bool isLimitMaxConnection() const
    {
        return m_iCurConns + 1 > m_iMaxConns;
    }
    uint32_t getNowConnection() const
    {
        return m_iCurConns;
    }
    void decreaseNowConnection()
    {
        --m_iCurConns;
    }
    void increaseNowConnection()
    {
        ++m_iCurConns;
    }

    template<class T>
    void setHandle(uint32_t iThreadNum)
    {
        for (uint32_t i = 0; i < iThreadNum; ++i) {
            HandlePtr handle(new T());
            handle->setBindAdapter(this);
            m_vHandles.push_back(handle);
        }
    }

    vector<HandlePtr> &getHandles()
    {
        return m_vHandles;
    }

protected:
    NetServer *m_pNetServer;
    string          m_sServiceName;

    CEndpoint       m_listenEndpoint;
    uint32_t		m_iCurConns;
    uint32_t        m_iMaxConns;
    uint32_t        m_iQueueCapacity;
    uint32_t        m_iQueueTimeout;
    string          m_sProtocolName;
    bool			m_bIsMfwProtocol;
    protocol_functor m_fnProtocol;
    CSocket         m_servsock;

    CThreadQueue<tagRecvData *> m_recvQueue;
    vector<HandlePtr>           m_vHandles;
};

class Connection
{
public:
    Connection(BindAdapter *pBindAdapter, uint32_t iTimeoutMS, int fd, const string &sIp, uint16_t iPort, bool bUdp = false);

    bool isUdp() const
    {
        return m_bIsUdp;
    }
    void setUid(uint32_t uid)
    {
        m_iUid = uid;
    }
    uint32_t getUid() const
    {
        return m_iUid;
    }

    BindAdapter *getBindAdapter()
    {
        return m_pBindAdapter;
    }
    const string &getIp() const
    {
        return m_sIp;
    }
    uint16_t getPort() const
    {
        return m_iPort;
    }
    uint16_t getBindPort() const
    {
        return m_iBindPort;
    }
    uint32_t getTimeout() const
    {
        return m_iTimeoutMS;
    }
    int getfd() const
    {
        return m_clisock.getfd();
    }

    bool setClose();
    void close();
    int send(const string &buffer, const string &ip, uint16_t port, bool bEncode=true);
    int send();
    int recv();

private:
    int parseProtocol();

private:
    BindAdapter         *m_pBindAdapter;
    bool				m_bIsUdp;
    CSocket           	m_clisock;
    uint32_t            m_iUid;
    uint32_t            m_iTimeoutMS;
    string              m_sIp;
    uint16_t            m_iPort;
    string              m_sRecvBuffer;
    string              m_sSendBuffer;
    bool                m_bClose;
    uint16_t			m_iBindPort;
    bool                m_bHandshake;
};

class ConnectionManager
{
public:
    explicit ConnectionManager(NetThread *pNetThread) : m_pNetThread(pNetThread), m_iLastUid(0) {}
    ~ConnectionManager();

    Connection *get(uint32_t uid);
    void add(Connection *cPtr);
    void del(uint32_t uid);
    void refresh(Connection *cPtr);
    uint64_t checkTimeout(uint64_t iNowMS);

private:
    NetThread						*m_pNetThread;
    uint32_t						m_iLastUid;
    map<uint32_t, Connection *>		m_mConnection;
    CTimeQueue<uint32_t, uint64_t>	m_timeQueue;
};

class NetThread : public CThreadBase
{
    enum {
        ET_NET    = 0,
        ET_NOTIFY = 1,
        ET_LISTEN = 2,
        ET_UDP	  = 3,
    };
public:
    NetThread() : m_connMgr(this) {}
    ~NetThread();

    void bind(const BindAdapterPtr &pBindAdapter);
    BindAdapterPtr getBindAdapter(const string &sServiceName);
    map<int, BindAdapterPtr> &getAllBindAdapter()
    {
        return m_mBindAdapter;
    }
    void initEpoll();
    void closeConnection(uint32_t uid);
    void sendResponse(uint32_t uid, const string &sData, const string &sIp, uint16_t iPort);
    void sendResponse(uint32_t uid, const string &sData);

    Connection *getConnection(uint32_t uid)
    {
        return m_connMgr.get(uid);
    }
    void addTcpConnection(Connection *cPtr);
    void addUdpConnection(Connection *cPtr);
    void delConnection(Connection *cPtr);

    virtual void run();
    void terminate();

private:
    void processNotify();
    void processNet(const epoll_event &ev);
    void processAccept(const epoll_event &ev);
    void processUDP(const epoll_event &ev);

private:
    map<int, BindAdapterPtr>    m_mBindAdapter;
    CEpoller                  	m_epoller;
    ConnectionManager           m_connMgr;
    CThreadQueue<tagSendData *>	m_sendQueue;

    map<uint64_t, uint16_t>		m_mUdpPort;
    map<uint16_t, uint64_t>		m_mPortUdp;
};

class NetServer
{
public:
    enum {
        PACKET_LESS = 0,
        PACKET_FULL = 1,
        PACKET_ERR  = -1,
    };

public:
    NetServer();
    ~NetServer();

    void bind(const BindAdapterPtr &pBindAdapter);
    void launch();
    void waitNotify(uint32_t iTimeMs);
    void terminate();
    bool isTerminate() const
    {
        return m_bTerminate;
    }
    NetThread *getNetThread()
    {
        return m_netThread;
    }

private:
    NetThread					*m_netThread;
    bool						m_bTerminate;
    CNotifier					m_notifier;
};

}

#endif
