#include "service/NetServer.h"
#include "service/ApplicationConfig.h"
#include "service/MfwProtocol.h"
#include "util/util_log.h"
#include "util/util_string.h"
#include <cassert>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "util/util_byteconvert.h"
#include "Websocket.h"

namespace mfw
{

#define H64(x) (((uint64_t)x) << 32)

void Handle::sendResponse(uint32_t uid, const string &sData, const string &sIp, uint16_t iPort)
{
    m_pBindAdapter->getNetServer()->getNetThread()->sendResponse(uid, sData, sIp, iPort);
}

void Handle::closeConnection(uint32_t uid)
{
    m_pBindAdapter->getNetServer()->getNetThread()->closeConnection(uid);
}

void Handle::run()
{
    if (!initialize()) return;

    while (!m_pBindAdapter->getNetServer()->isTerminate()) {
        tagRecvData *pstRecvData = NULL;
        try {
            while (m_pBindAdapter->waitForRecvQueue(pstRecvData, -1)) {
                tagRecvData &stRecvData = *pstRecvData;
                stRecvData.adapter = m_pBindAdapter;

                if (stRecvData.isOverload) {
                    handleOverload(stRecvData);
                } else if (stRecvData.isClosed) {
                    handleClose(stRecvData);
                } else {
                    uint64_t iNowMS = UtilTime::getNowMS();
                    if (iNowMS > stRecvData.recvTimeStamp + m_pBindAdapter->getQueueTimeout()) {
                        handleTimeout(stRecvData);
                    } else {
                        handle(stRecvData);
                    }
                }

                delete pstRecvData;
                pstRecvData = NULL;
            }
        } catch (std::exception &e) {
            if (pstRecvData) {
                closeConnection(pstRecvData->uid);
                delete pstRecvData;
                pstRecvData = NULL;
            }
            MFW_ERROR("handle request exception: " << e.what());
        }
    }
}

BindAdapter::BindAdapter(NetServer *pNetServer, const string &sServiceName) :
    m_pNetServer(pNetServer),
    m_sServiceName(sServiceName),
    m_iCurConns(0),
    m_iMaxConns(DEFAULT_MAX_SERVER_CONNECTION),
    m_iQueueCapacity(DEFAULT_MAX_SERVER_QUEUE_CAPACITY),
    m_iQueueTimeout(DEFAULT_MAX_SERVER_QUEUE_TIMEOUT),
    m_sProtocolName("mfw"),
    m_bIsMfwProtocol(true),
    m_fnProtocol(ServerSideProtocol::mfw_protocol)
{
}

void BindAdapter::insertRecvQueue(tagRecvData *pstRecvData, bool bCheckOverload)
{
    if (!bCheckOverload) {
        m_recvQueue.enqueue(pstRecvData);
        return;
    }

    uint32_t iRecvBufferSize = m_recvQueue.size();
    if (iRecvBufferSize <= (m_iQueueCapacity / 2)) {
        m_recvQueue.enqueue(pstRecvData);
    } else if (iRecvBufferSize > (m_iQueueCapacity / 2) && (iRecvBufferSize < m_iQueueCapacity) && m_iQueueCapacity > 0) {
        pstRecvData->isOverload = true;
        m_recvQueue.enqueue(pstRecvData);
    } else {
        delete pstRecvData;
    }
}

void BindAdapter::wakeupRecvQueue()
{
    m_recvQueue.wakeup();
}

bool BindAdapter::waitForRecvQueue(tagRecvData* &pstRecvData, int64_t iWaitTime)
{
    return m_recvQueue.dequeue(pstRecvData, iWaitTime);
}

Connection::Connection(BindAdapter *pBindAdapter, uint32_t iTimeoutMS, int fd, const string &sIp, uint16_t iPort, bool bUdp) :
    m_pBindAdapter(pBindAdapter),
    m_bIsUdp(bUdp),
    m_iUid(0),
    m_iTimeoutMS(iTimeoutMS),
    m_iPort(0),
    m_bClose(false),
    m_iBindPort(0),
    m_bHandshake(false)
{
    m_clisock.init(fd, true);
    if (m_iTimeoutMS < MIN_SERVER_CONNECTION_IDLE_TIME) {
        m_iTimeoutMS = MIN_SERVER_CONNECTION_IDLE_TIME;
    }

    if (isUdp()) {
        m_iBindPort = iPort;
    } else {
        m_sIp = sIp;
        m_iPort = iPort;
    }
}

void Connection::close()
{
    m_clisock.close();
}

int Connection::parseProtocol()
{
    try {
        const char *pInBegin = m_sRecvBuffer.c_str();
        const char *pInEnd = pInBegin + m_sRecvBuffer.size();
        while (pInBegin < pInEnd) {
            string sOut;
            int ret = -1;
            if (!m_pBindAdapter->getEndpoint().isWebsocket()) {
                ret = m_pBindAdapter->getProtocol()(pInBegin, pInEnd, pInBegin, sOut);
            } else if (!m_bHandshake) {
                ret = Websocket::handshake(pInBegin, pInEnd, pInBegin, sOut);
            } else {
                ret = Websocket::parseProtocol(pInBegin, pInEnd, pInBegin, sOut);
            }

            if (ret == NetServer::PACKET_LESS) {
                break;
            } else if (ret == NetServer::PACKET_FULL) {
                if (m_pBindAdapter->getEndpoint().isWebsocket() && !m_bHandshake) {
                    send(sOut, m_sIp, m_iPort, false);
                    m_bHandshake = true;
                } else {
                    tagRecvData *pstRecvData = new tagRecvData();
                    swap(pstRecvData->buffer, sOut);
                    pstRecvData->ip               = m_sIp;
                    pstRecvData->port             = m_iPort;
                    pstRecvData->recvTimeStamp    = UtilTime::getNowMS();
                    pstRecvData->uid              = getUid();
                    pstRecvData->isOverload       = false;
                    pstRecvData->isClosed         = false;
                    m_pBindAdapter->insertRecvQueue(pstRecvData, true);
                }
            } else {
                if (ret == -1) {
                    MFW_ERROR("recv invalid packet, peer: " << m_sIp << ":" << m_iPort);
                }
                return -1;
            }
        }
        m_sRecvBuffer.erase(m_sRecvBuffer.begin(), m_sRecvBuffer.begin() + (pInBegin - m_sRecvBuffer.c_str()));
    } catch (std::exception &e) {
        MFW_ERROR("parse protocol exception:" << e.what());
        return -1;
    }
    return 0;
}

int Connection::recv()
{
    if (!isUdp()) {
        char buf[TCP_RECV_BUFFER_SIZE];
        while (true) {
            int n = m_clisock.recv(buf, sizeof(buf));
            if (n < 0 && errno == EAGAIN) {
                break;
            } else if (n <= 0) {
                MFW_DEBUG("close connection by peer: " << m_sIp << ":" << m_iPort << ", ret: " << n << ",errno: " << errno);
                return -1;
            }
            m_sRecvBuffer.append(buf, n);
            if (n < (int)sizeof(buf)) {
                break;
            }
        }
        return parseProtocol();
    } else {
        char buf[UDP_RECV_BUFFER_SIZE];
        while (true) {
            int n = m_clisock.recvfrom(buf, sizeof(buf), m_sIp, m_iPort);
            if (n <= 0) {
                break;
            }
            m_sRecvBuffer.assign(buf, n);
            parseProtocol();
            m_sRecvBuffer.clear();
        }
    }
    return 0;
}

int Connection::send(const string &buffer, const string &ip, uint16_t port, bool bEncode)
{
    if (isUdp()) {
        int n = m_clisock.sendto(buffer.c_str(), buffer.length(), ip, port);
        if (n != (int)buffer.length()) {
            MFW_ERROR("udp send error, peer: " << ip << ":" << port);
        }
        return 0;
    }

    if (m_pBindAdapter->getEndpoint().isWebsocket() && bEncode) {
        m_sSendBuffer.append(Websocket::encodeProtocol(buffer));
    } else {
        m_sSendBuffer.append(buffer);
    }
    return send();
}

int Connection::send()
{
    if (!m_sSendBuffer.empty()) {
        int n = m_clisock.send(m_sSendBuffer.c_str(), m_sSendBuffer.length());
        if (n <= 0) {
            if (errno != EAGAIN) {
                MFW_DEBUG("close connection by peer: " << m_sIp << ":" << m_iPort << ", ret: " << n << ",errno: " << errno);
                return -1;
            }
        } else {
            m_sSendBuffer.erase(m_sSendBuffer.begin(), m_sSendBuffer.begin() + n);
            if (m_sSendBuffer.capacity() - m_sSendBuffer.length() > TCP_SEND_MAX_RESERVE_SIZE) {
                m_sSendBuffer.reserve(std::max(m_sSendBuffer.length(), (size_t)TCP_SEND_MIN_RESERVE_SIZE));
            }
        }
    }

    if (m_bClose && m_sSendBuffer.empty()) {
        MFW_DEBUG("close connection by server, peer: " << m_sIp << ":" << m_iPort);
        return -1;
    }
    return 0;
}

bool Connection::setClose()
{
    m_bClose = true;
    if (m_sSendBuffer.empty()) {
        return true;
    }
    return false;
}

Connection *ConnectionManager::get(uint32_t uid)
{
    map<uint32_t, Connection *>::iterator it = m_mConnection.find(uid);
    if (it != m_mConnection.end()) {
        return it->second;
    }
    return NULL;
}

void ConnectionManager::add(Connection *cPtr)
{
    while (true) {
        ++m_iLastUid;
        if (m_iLastUid == 0) {
            ++m_iLastUid;
        }
        if (m_mConnection.find(m_iLastUid) == m_mConnection.end()) {
            break;
        }
    }

    cPtr->setUid(m_iLastUid);
    m_mConnection[m_iLastUid] = cPtr;
    refresh(cPtr);
}

void ConnectionManager::del(uint32_t uid)
{
    map<uint32_t, Connection *>::iterator it = m_mConnection.find(uid);
    if (it != m_mConnection.end()) {
        Connection *cPtr = it->second;
        m_timeQueue.del(uid);
        m_mConnection.erase(it);
        delete cPtr;
    }
}

void ConnectionManager::refresh(Connection *cPtr)
{
    m_timeQueue.add(cPtr->getUid(), cPtr->getTimeout() + UtilTime::getNowMS());
}

uint64_t ConnectionManager::checkTimeout(uint64_t iNowMS)
{
    uint32_t uid = 0;
    uint64_t iTimeMS = 0;
    while (m_timeQueue.peek(uid, iTimeMS)) {
        if (iTimeMS > iNowMS) {
            return iTimeMS - iNowMS;
        }

        Connection *cPtr = get(uid);
        MFW_DEBUG("connection idle timeout, peer: " << cPtr->getIp() << ":" << cPtr->getPort());
        m_pNetThread->delConnection(cPtr);
    }
    return (uint64_t)-1;
}

ConnectionManager::~ConnectionManager()
{
    map<uint32_t, Connection *>::iterator it = m_mConnection.begin();
    while (it != m_mConnection.end()) {
        delete it->second;
        ++it;
    }
}

NetThread::~NetThread()
{
    for (map<int, BindAdapterPtr>::iterator first = m_mBindAdapter.begin(), last = m_mBindAdapter.end(); first != last; ++first) {
        int fd = first->first;
        ::close(fd);
    }

    tagSendData *pstSendData = NULL;
    while (m_sendQueue.dequeue(pstSendData, 0)) {
        delete pstSendData;
    }
}

void NetThread::bind(const BindAdapterPtr &pBindAdapter)
{
    if (getBindAdapter(pBindAdapter->getServiceName())) {
        throw std::runtime_error("already bind: " + pBindAdapter->getServiceName());
    }

    const CEndpoint &ep = pBindAdapter->getEndpoint();
    CSocket &servsock = pBindAdapter->getSocket();
    if (ep.isTcp()) {
        servsock.create(CSocket::SockType_Tcp4);
        servsock.bind(ep.getHost(), ep.getPort());
    } else if (ep.isUnix()) {
        servsock.create(CSocket::SockType_Unix);
        servsock.bind(ep.getHost());
    } else {
        servsock.create(CSocket::SockType_Udp4);
        servsock.bind(ep.getHost(), ep.getPort());
    }

    if (ep.isTcp() || ep.isUnix()) {
        if (ep.isTcp()) {
            servsock.setTcpNoDelay();
        }
        servsock.listen();
    }
    servsock.setblock(false);

    m_mBindAdapter[servsock.getfd()] = pBindAdapter;
}

BindAdapterPtr NetThread::getBindAdapter(const string &sServiceName)
{
    for (map<int, BindAdapterPtr>::iterator first = m_mBindAdapter.begin(), last = m_mBindAdapter.end(); first != last; ++first) {
        BindAdapterPtr &pBindAdapter = first->second;
        if (pBindAdapter->getServiceName() == sServiceName) {
            return pBindAdapter;
        }
    }
    return BindAdapterPtr();
}

void NetThread::initEpoll()
{
    m_epoller.setEnableNotify(H64(ET_NOTIFY));

    for (map<int, BindAdapterPtr>::iterator first = m_mBindAdapter.begin(), last = m_mBindAdapter.end(); first != last; ++first) {
        int fd = first->first;
        BindAdapterPtr &pBindAdapter = first->second;
        const CEndpoint &ep = pBindAdapter->getEndpoint();
        if (ep.isTcp() || ep.isUnix()) {
            m_epoller.add(fd, H64(ET_LISTEN) | fd, EPOLLIN);
        } else if (ep.isUdp()) {
            m_epoller.add(fd, H64(ET_UDP) | fd, EPOLLIN);
        }
    }
}

void NetThread::addTcpConnection(Connection *cPtr)
{
    cPtr->getBindAdapter()->increaseNowConnection();
    m_connMgr.add(cPtr);
    m_epoller.add(cPtr->getfd(), cPtr->getUid(), EPOLLIN | EPOLLOUT);
}

void NetThread::addUdpConnection(Connection *cPtr)
{
    cPtr->getBindAdapter()->increaseNowConnection();
    m_connMgr.add(cPtr);
    m_epoller.add(cPtr->getfd(), cPtr->getUid(), EPOLLIN | EPOLLOUT);
}

void NetThread::delConnection(Connection *cPtr)
{
    tagRecvData *pstRecvData = new tagRecvData();
    pstRecvData->uid        = cPtr->getUid();
    pstRecvData->ip         = cPtr->getIp();
    pstRecvData->port       = cPtr->getPort();
    pstRecvData->isClosed   = true;
    pstRecvData->isOverload = false;
    pstRecvData->recvTimeStamp = UtilTime::getNowMS();

    cPtr->getBindAdapter()->insertRecvQueue(pstRecvData, false);
    cPtr->getBindAdapter()->decreaseNowConnection();

    if (cPtr->isUdp()) {
        map<uint16_t, uint64_t>::const_iterator it = m_mPortUdp.find(cPtr->getBindPort());
        if (it != m_mPortUdp.end()) {
            m_mUdpPort.erase(it->second);
        }
        m_mPortUdp.erase(cPtr->getBindPort());
    }

    m_epoller.del(cPtr->getfd());
    cPtr->close();

    m_connMgr.del(cPtr->getUid());
}

void NetThread::terminate()
{
    getThread().terminate();
    m_epoller.notify(H64(ET_NOTIFY));
    getThread().join();

    map<int, BindAdapterPtr> &mBindAdapter = getAllBindAdapter();
    for (map<int, BindAdapterPtr>::iterator first = mBindAdapter.begin(), last = mBindAdapter.end(); first != last; ++first) {
        BindAdapterPtr &pBindAdapter = first->second;
        pBindAdapter->wakeupRecvQueue();
    }
    for (map<int, BindAdapterPtr>::iterator first = mBindAdapter.begin(), last = mBindAdapter.end(); first != last; ++first) {
        BindAdapterPtr &pBindAdapter = first->second;
        vector<HandlePtr> vHandles = pBindAdapter->getHandles();
        for (unsigned i = 0; i < vHandles.size(); ++i) {
            HandlePtr &handle = vHandles[i];
            handle->getThread().join();
        }
    }
}

void NetThread::closeConnection(uint32_t uid)
{
    tagSendData *pstSendData = new tagSendData();
    pstSendData->uid = uid;
    pstSendData->cmd = 'c';
    m_sendQueue.enqueue(pstSendData);
    m_epoller.notify(H64(ET_NOTIFY));
}

void NetThread::sendResponse(uint32_t uid, const string &sData, const string &sIp, uint16_t iPort)
{
    if (getThread().isTerminate()) {
        return;
    }

    tagSendData *pstSendData = new tagSendData();
    pstSendData->uid = uid;
    pstSendData->cmd = 's';
    pstSendData->buffer = sData;
    pstSendData->ip = sIp;
    pstSendData->port = iPort;
    m_sendQueue.enqueue(pstSendData);
    m_epoller.notify(H64(ET_NOTIFY));
}

void NetThread::sendResponse(uint32_t uid, const string &sData)
{
    if (getThread().isTerminate()) {
        return;
    }

    tagSendData *pstSendData = new tagSendData();
    pstSendData->uid = uid;
    pstSendData->cmd = 's';
    pstSendData->buffer = sData;
    m_sendQueue.enqueue(pstSendData);
    m_epoller.notify(H64(ET_NOTIFY));
}

void NetThread::processNotify()
{
    tagSendData *pstSendData = NULL;
    while (m_sendQueue.dequeue(pstSendData, 0)) {
        switch(pstSendData->cmd) {
        case 'c': {
            Connection *cPtr = getConnection(pstSendData->uid);
            if (cPtr) {
                if (cPtr->setClose()) {
                    delConnection(cPtr);
                }
            }
            break;
        }
        case 's': {
            Connection *cPtr = getConnection(pstSendData->uid);
            if (cPtr) {
                int ret = 0;
                if (pstSendData->ip.empty() || pstSendData->port == 0) {
                    ret = cPtr->send(pstSendData->buffer, cPtr->getIp(), cPtr->getPort());
                } else {
                    ret = cPtr->send(pstSendData->buffer, pstSendData->ip, pstSendData->port);
                }

                if(ret < 0) {
                    delConnection(cPtr);
                }
            }
            break;
        }
        }
        delete pstSendData;
    }
}

void NetThread::processNet(const epoll_event &ev)
{
    uint32_t uid = (uint32_t)ev.data.u64;
    Connection *cPtr = getConnection(uid);
    if (!cPtr) {
        MFW_DEBUG("connection not exist: " << uid);
        return;
    }

    if ((ev.events & EPOLLERR) || (ev.events & EPOLLHUP)) {
        delConnection(cPtr);
        return;
    }

    if (ev.events & EPOLLIN) {
        int ret = cPtr->recv();
        if(ret < 0) {
            delConnection(cPtr);
            return;
        }
    }

    if (ev.events & EPOLLOUT) {
        int ret = cPtr->send();
        if (ret < 0) {
            delConnection(cPtr);
            return;
        }
    }

    m_connMgr.refresh(cPtr);
}

void NetThread::processAccept(const epoll_event &ev)
{
    if (!(ev.events & EPOLLIN)) {
        return;
    }

    int fd = (int)(uint32_t)ev.data.u64;
    map<int, BindAdapterPtr>::iterator it = m_mBindAdapter.find(fd);
    if (it == m_mBindAdapter.end()) {
        return;
    }

    BindAdapter *pBindAdapter = it->second.get();
    CSocket &servsock = pBindAdapter->getSocket();
    while (true) {
        struct sockaddr_storage sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        socklen_t socklen = sizeof(sockaddr);

        CSocket clisock;
        if (!servsock.accept(clisock, (struct sockaddr *)&sockaddr, &socklen)) {
            break;
        }

        string ip;
        uint16_t port = 0;
        UtilNet::decodeAddr(sockaddr, ip, port);

        if (pBindAdapter->isLimitMaxConnection()) {
            MFW_ERROR("accept exceed max connection, peer: " << ip << ":" << port);
            clisock.close();
        } else {
            MFW_DEBUG("accept peer: " << ip << ":" << port);
            clisock.setblock(false);
            if (port != 0) {
                clisock.setTcpNoDelay();
            }

            clisock.setOwned(false);
            Connection *cPtr = new Connection(pBindAdapter, pBindAdapter->getEndpoint().getTimeout(), clisock.getfd(), ip, port);
            addTcpConnection(cPtr);
        }
    }
}

void NetThread::processUDP(const epoll_event &ev)
{
    if (!(ev.events & EPOLLIN)) {
        return;
    }

    int fd = (int)(uint32_t)ev.data.u64;
    map<int, BindAdapterPtr>::iterator it = m_mBindAdapter.find(fd);
    if (it == m_mBindAdapter.end()) {
        return;
    }

    BindAdapter *pBindAdapter = it->second.get();
    CSocket &servsock = pBindAdapter->getSocket();
    const CEndpoint &endpoint = pBindAdapter->getEndpoint();

    while (true) {
        struct sockaddr_storage sockaddr;
        memset(&sockaddr, 0, sizeof(sockaddr));
        socklen_t socklen = sizeof(sockaddr);

        char buf[10];
        int n = servsock.recvfrom(buf, sizeof(buf), (struct sockaddr *)&sockaddr, &socklen);
        if (n <= 0) {
            break;
        }

        ByteBuffer bb;
        bb.resize(n);
        memcpy((char*)bb.contents(), buf, n);

        string ip;
        uint16_t port = 0;
        UtilNet::decodeAddr(sockaddr, ip, port);

        uint8_t cmd = 0;
        uint64_t uid = 0;

        try {
            bb >> cmd >> uid;
        } catch(const ByteBufferException &ex) {
            MFW_ERROR("udp accept invalid packet: " << ip << ":" << port << ", payload:" << string(buf, n));
            bb.hexlike();
            continue;
        }

        if (cmd != 0x01 || uid == 0) {
            bb.hexlike();
            MFW_ERROR("udp accept invalid packet: " << ip << ":" << port << ", payload:" << string(buf, n));
            continue;
        }

        uint16_t bport = 0;
        map<uint64_t, uint16_t>::const_iterator it = m_mUdpPort.find(uid);
        if (it == m_mUdpPort.end()) {
            if (pBindAdapter->isLimitMaxConnection()) {
                MFW_ERROR("udp accept exceed max connection, peer: " << ip << ":" << port);
                continue;
            } else {
                CSocket newsock;
                newsock.create(CSocket::SockType_Udp4);
                newsock.bind(endpoint.getHost(), 0);
                newsock.setblock(false);

                struct sockaddr_storage newsockaddr;
                memset(&newsockaddr, 0, sizeof(newsockaddr));
                socklen_t newsocklen = sizeof(newsockaddr);

                if (getsockname(newsock.getfd(), (struct sockaddr *)&newsockaddr, &newsocklen) != 0) {
                    MFW_DEBUG("udp bind new port failed, errno:" << string(strerror(errno)));
                    continue;
                }

                string bip;
                UtilNet::decodeAddr(newsockaddr, bip, bport);

                Connection *cPtr = new Connection(pBindAdapter, pBindAdapter->getEndpoint().getTimeout(), newsock.getfd(), bip, bport, true);
                addUdpConnection(cPtr);

                m_mUdpPort[uid] = bport;
                m_mPortUdp[bport] = uid;

                MFW_DEBUG("udp accept peer: " << ip << ":" << port << ", bind: " << bip << ":" << bport);
            }
        } else {
            bport = it->second;
        }

        bb.clear();
        bb << (uint8_t)(cmd+1) << bport;
        servsock.sendto(bb.contents(), bb.size(), (struct sockaddr *)&sockaddr, socklen);
    }
}

void NetThread::run()
{
    while (!getThread().isTerminate()) {
        uint64_t iSleepMS = m_connMgr.checkTimeout(UtilTime::getNowMS());
        if (iSleepMS < 50) iSleepMS = 50;
        if (iSleepMS > 86400 * 1000) iSleepMS = 86400 * 1000;

        int num = m_epoller.wait(iSleepMS);
        for (int i = 0; i < num; ++i) {
            try {
                const epoll_event &ev = m_epoller.get(i);
                uint32_t type = ev.data.u64 >> 32;
                switch (type) {
                case ET_NET:
                    processNet(ev);
                    break;
                case ET_NOTIFY:
                    processNotify();
                    break;
                case ET_LISTEN:
                    processAccept(ev);
                    break;
                case ET_UDP:
                    processUDP(ev);
                    break;
                }
            } catch (std::exception &e) {
                MFW_ERROR("server netthread exception: " << e.what());
            }
        }
    }
}

NetServer::NetServer()
    : m_netThread(NULL)
    , m_bTerminate(false)
{
    m_netThread = new NetThread();
}

NetServer::~NetServer()
{
    m_netThread->terminate();

    delete m_netThread;
    m_netThread = NULL;
}

void NetServer::bind(const BindAdapterPtr &pBindAdapter)
{
    m_netThread->bind(pBindAdapter);
}

void NetServer::launch()
{
    map<int, BindAdapterPtr> &mBindAdapter = m_netThread->getAllBindAdapter();
    for (map<int, BindAdapterPtr>::iterator first = mBindAdapter.begin(), last = mBindAdapter.end(); first != last; ++first) {
        BindAdapterPtr &pBindAdapter = first->second;
        vector<HandlePtr> vHandles = pBindAdapter->getHandles();
        for (unsigned i = 0; i < vHandles.size(); ++i) {
            HandlePtr &handle = vHandles[i];
            handle->start("handle-" + UtilString::tostr(i) + "-" + pBindAdapter->getServiceName());
        }
    }

    m_netThread->initEpoll();
    m_netThread->start("server-net");
}

void NetServer::waitNotify(uint32_t iTimeMs)
{
    CLockGuard<CNotifier> lock(m_notifier);
    if (iTimeMs != (uint32_t)-1) {
        m_notifier.timedwait(iTimeMs);
    } else {
        m_notifier.wait();
    }
}

void NetServer::terminate()
{
    if (!m_bTerminate) {
        m_bTerminate = true;
        CLockGuard<CNotifier> lock(m_notifier);
        m_notifier.broadcast();
    }
}

}
