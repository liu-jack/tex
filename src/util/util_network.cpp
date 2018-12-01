#include "util_network.h"
#include "util_string.h"
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
using namespace std;

namespace mfw
{

bool UtilNet::isIPv4(const string &ip)
{
    struct sockaddr_in addr;
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        return false;
    }
    return true;
}

bool UtilNet::isIPv6(const string &ip)
{
    struct sockaddr_in6 addr;
    if (inet_pton(AF_INET6, ip.c_str(), &addr.sin6_addr) != 1) {
        return false;
    }
    return true;
}

void UtilNet::encodeIPv4(const string &ip, uint16_t port, struct sockaddr_in &addr)
{
    memset(&addr, 0, sizeof(addr));
    int ret = inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    if (ret != 1) {
        throw std::runtime_error("inet_pton: invalid ipv4 " + ip);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
}

void UtilNet::decodeIPv4(const struct sockaddr_in &addr, string &ip, uint16_t &port)
{
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)) == NULL) {
        throw std::runtime_error("inet_ntop: invalid ipv4 addr");
    }
    ip = buf;
    port = ntohs(addr.sin_port);
}

void UtilNet::encodeIPv6(const string &ip, uint16_t port, struct sockaddr_in6 &addr)
{
    memset(&addr, 0, sizeof(addr));
    int ret = inet_pton(AF_INET6, ip.c_str(), &addr.sin6_addr);
    if (ret != 1) {
        throw std::runtime_error("inet_pton: invalid ipv6 " + ip);
    }
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port);
}

void UtilNet::decodeIPv6(const struct sockaddr_in6 &addr, string &ip, uint16_t &port)
{
    char buf[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &addr.sin6_addr, buf, sizeof(buf)) == NULL) {
        throw std::runtime_error("inet_ntop: invalid ipv6 addr");
    }
    ip = buf;
    port = ntohs(addr.sin6_port);
}

void UtilNet::encodeUnix(const string &sPath, struct sockaddr_un &addr)
{
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sPath.c_str(), sizeof(addr.sun_path));
    addr.sun_path[sizeof(addr.sun_path) - 1] = 0;
}

void UtilNet::decodeUnix(const struct sockaddr_un &addr, string &sPath)
{
    sPath = addr.sun_path;
}

void UtilNet::encodeAddr(const string &ip, uint16_t port, struct sockaddr_storage &addr)
{
    if (isIPv4(ip)) {
        encodeIPv4(ip, port, (struct sockaddr_in &)addr);
    } else if (isIPv6(ip)) {
        encodeIPv6(ip, port, (struct sockaddr_in6 &)addr);
    } else {
        struct sockaddr_un tmp;
        encodeUnix(ip, tmp);
        memcpy(&addr, &tmp, sizeof(tmp));
    }
}

void UtilNet::decodeAddr(const struct sockaddr_storage &addr, string &ip, uint16_t &port)
{
    if (addr.ss_family == AF_INET) {
        decodeIPv4((const struct sockaddr_in &)addr, ip, port);
    } else if (addr.ss_family == AF_INET6) {
        decodeIPv6((const struct sockaddr_in6 &)addr, ip, port);
    } else if (addr.ss_family == AF_UNIX) {
        decodeUnix((const struct sockaddr_un &)addr, ip);
        port = 0;
    } else {
        throw std::runtime_error("unsupported addr type");
    }
}

CEndpoint::CEndpoint(EndpointType type, const string &sHost, uint16_t iPort, uint32_t iTimeout, bool bWebsocket)
{
    m_type = type;
    m_sHost = sHost;
    m_iPort = iPort;
    m_iTimeout = iTimeout;
    m_bWebsocket = bWebsocket;
    _gendesc();
}

CEndpoint::CEndpoint(EndpointType type, const string &sPath, uint32_t iTimeout, bool bWebsocket)
{
    m_type = type;
    m_sHost = sPath;
    m_iPort = 0;
    m_iTimeout = iTimeout;
    m_bWebsocket = bWebsocket;
    _gendesc();
}

void CEndpoint::reset()
{
    m_type = EndpointType_Invalid;
    m_sDesc.clear();
    m_sHost.clear();
    m_iPort = 0;
    m_iTimeout = 0;
    m_bWebsocket = false;
}

void CEndpoint::parse(const string &sDesc)
{
    if (!_parse(sDesc)) {
        reset();
        throw std::runtime_error("invalid endpoint desc: " + sDesc);
    }
}

bool CEndpoint::parseNoThrow(const string &sDesc)
{
    if (!_parse(sDesc)) {
        reset();
        return false;
    }
    return true;
}

bool CEndpoint::_parse(const string &sDesc)
{
    vector<string> v = UtilString::splitString(sDesc, " ");
    if (v.size() < 3 || v.size() % 2 != 1) {
        return false;
    }

    reset();
    if (v[0] == "tcp") {
        m_type = EndpointType_Tcp4;
    } else if (v[0] == "ws") {
        m_type = EndpointType_Tcp4;
        m_bWebsocket = true;
    } else if (v[0] == "udp") {
        m_type = EndpointType_Udp4;
    } else if (v[0] == "unix") {
        m_type = EndpointType_Unix;
    } else {
        return false;
    }

    for (unsigned i = 1; i < v.size(); i += 2) {
        const string &opt = v[i];
        const string &arg = v[i + 1];
        if (opt.length() != 2 || opt[0] != '-') {
            return false;
        }
        if (opt[1] == 'h') {
            if ((isTcp() || isUdp()) && !UtilNet::isIPv4(arg)) {
                return false;
            }
            m_sHost = arg;
        } else if (opt[1] == 'p') {
            if (!isTcp() && !isUdp()) {
                return false;
            }
            m_iPort = UtilString::strto<uint16_t>(arg);
        } else if (opt[1] == 't') {
            m_iTimeout = UtilString::strto<uint32_t>(arg);
        } else {
            continue;
        }
    }
    if (m_sHost.empty()) {
        return false;
    }
    _gendesc();
    return true;
}

void CEndpoint::_gendesc()
{
    m_sDesc = isTcp() ? isWebsocket() ? "ws" : "tcp" : isUdp() ? "udp" : "unix";
    m_sDesc += " -h " + m_sHost;
    if (m_iPort != 0) {
        m_sDesc += " -p " + UtilString::tostr(m_iPort);
    }
    if (m_iTimeout != 0) {
        m_sDesc += " -t " + UtilString::tostr(m_iTimeout);
    }
}

void CSocket::reset()
{
    if (m_bOwned) {
        close();
    }
}

void CSocket::init(int sock, bool bOwned)
{
    reset();
    m_sockfd = sock;
    m_bOwned = bOwned;
}

void CSocket::create(SockType type)
{
    int iDomain = 0;
    int iSocketType = 0;
    switch (type) {
    case SockType_Tcp4:
        iDomain = AF_INET;
        iSocketType = SOCK_STREAM;
        break;
    case SockType_Udp4:
        iDomain = AF_INET;
        iSocketType = SOCK_DGRAM;
        break;
    case SockType_Unix:
        iDomain = AF_UNIX;
        iSocketType = SOCK_STREAM;
        break;
    }

    reset();
    m_sockfd = socket(iDomain, iSocketType, 0);
    if (m_sockfd < 0) {
        throw std::runtime_error("socket: " + string(strerror(errno)));
    }
}

void CSocket::close()
{
    if (m_sockfd >= 0) {
        ::close(m_sockfd);
        m_sockfd = -1;
    }
}

bool CSocket::connect(const string &ip, uint16_t port)
{
    struct sockaddr_storage addr;
    UtilNet::encodeAddr(ip, port, addr);
    return connect((struct sockaddr *)&addr, sizeof(addr));
}

bool CSocket::connect(const string &sPath)
{
    struct sockaddr_un addr;
    UtilNet::encodeUnix(sPath, addr);
    return connect((struct sockaddr *)&addr, sizeof(addr));
}

bool CSocket::connect(const struct sockaddr *addr, socklen_t len)
{
    int ret = ::connect(m_sockfd, addr, len);
    if (ret == 0) {
        return true;
    }
    if (errno != EINPROGRESS) {
        throw std::runtime_error("connect: " + string(strerror(errno)));
    }
    return false;
}

void CSocket::bind(const string &ip, uint16_t port)
{
    struct sockaddr_storage addr;
    UtilNet::encodeAddr(ip, port, addr);
    bind((struct sockaddr *)&addr, sizeof(addr));
}

void CSocket::bind(const string &sPath)
{
    unlink(sPath.c_str());

    struct sockaddr_un addr;
    UtilNet::encodeUnix(sPath, addr);
    bind((struct sockaddr *)&addr, sizeof(addr));
}

void CSocket::bind(const struct sockaddr *addr, socklen_t len)
{
    setsockopt(SOL_SOCKET, SO_REUSEADDR, 1, "SO_REUSEADDR");

    int ret = ::bind(m_sockfd, addr, len);
    if (ret != 0) {
        throw std::runtime_error("bind: " + string(strerror(errno)));
    }
}

void CSocket::listen(int backlog)
{
    int ret = ::listen(m_sockfd, backlog);
    if (ret != 0) {
        throw std::runtime_error("listen: " + string(strerror(errno)));
    }
}

bool CSocket::accept(CSocket &sock)
{
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    return accept(sock, (struct sockaddr *)&addr, &len);
}

bool CSocket::accept(CSocket &sock, struct sockaddr *addr, socklen_t *len)
{
    int fd = ::accept(m_sockfd, addr, len);
    if (fd < 0) {
        if (errno == EAGAIN) {
            return false;
        }
        throw std::runtime_error("accept: " + string(strerror(errno)));
    }
    sock.init(fd, true);
    return true;
}

void CSocket::setblock(bool bBlock)
{
    int val = fcntl(m_sockfd, F_GETFL, 0);
    if (val == -1) {
        throw std::runtime_error("fcntl(F_GETFL): " + string(strerror(errno)));
    }

    if(!bBlock) {
        val |= O_NONBLOCK;
    } else {
        val &= ~O_NONBLOCK;
    }

    if (fcntl(m_sockfd, F_SETFL, val) == -1) {
        throw std::runtime_error("fcntl(F_SETFL): " + string(strerror(errno)));
    }
}

void CSocket::setTcpNoDelay()
{
    setsockopt(IPPROTO_TCP, TCP_NODELAY, 1, "TCP_NODELAY");
}

void CSocket::setsockopt(int level, int optname, int optval, const char *name)
{
    setsockopt(level, optname, &optval, sizeof(optval), name);
}

void CSocket::setsockopt(int level, int optname, const void *optval, socklen_t optlen, const char *name)
{
    int ret = ::setsockopt(m_sockfd, level, optname, optval, optlen);
    if (ret != 0) {
        if (name != NULL) {
            throw std::runtime_error("setsockopt " + string(name) + ": " + string(strerror(errno)));
        } else {
            throw std::runtime_error("setsockopt: " + string(strerror(errno)));
        }
    }
}

int CSocket::getsockerror()
{
    int iCode = 0;
    getsockopt(SOL_SOCKET, SO_ERROR, iCode, "SO_ERROR");
    return iCode;
}

void CSocket::getsockopt(int level, int optname, int &optval, const char *name)
{
    socklen_t optlen = sizeof(optval);
    getsockopt(level, optname, &optval, &optlen, name);
}

void CSocket::getsockopt(int level, int optname, void *optval, socklen_t *optlen, const char *name)
{
    int ret = ::getsockopt(m_sockfd, level, optname, optval, optlen);
    if (ret != 0) {
        if (name != NULL) {
            throw std::runtime_error("getsockopt " + string(name) + ": " + string(strerror(errno)));
        } else {
            throw std::runtime_error("getsockopt: " + string(strerror(errno)));
        }
    }
}

int CSocket::recv(void *buf, uint32_t len)
{
    return ::recv(m_sockfd, buf, len, 0);
}

int CSocket::send(const void *buf, uint32_t len)
{
    return ::send(m_sockfd, buf, len, 0);
}

int CSocket::recvfrom(void *buf, uint32_t len, string &ip, uint16_t &port)
{
    sockaddr_storage addr;
    socklen_t socklen = sizeof(addr);
    int ret = recvfrom(buf, len, (struct sockaddr *)&addr, &socklen);
    if (ret >= 0) {
        UtilNet::decodeAddr(addr, ip, port);
    }
    return ret;
}

int CSocket::recvfrom(void *buf, uint32_t len, struct sockaddr *addr, socklen_t *socklen)
{
    return ::recvfrom(m_sockfd, buf, len, 0, addr, socklen);
}

int CSocket::sendto(const void *buf, uint32_t len, const string &ip, uint16_t port)
{
    sockaddr_storage addr;
    UtilNet::encodeAddr(ip, port, addr);
    return sendto(buf, len, (struct sockaddr *)&addr, sizeof(addr));
}

int CSocket::sendto(const void *buf, uint32_t len, const struct sockaddr *addr, socklen_t socklen)
{
    return ::sendto(m_sockfd, buf, len, 0, addr, socklen);
}

CEpoller::CEpoller(bool bEdgeTrigger)
    : m_bEdgeTrigger(bEdgeTrigger), m_epollfd(-1)
{
    m_epollfd = epoll_create(128);
    m_vEpollEvent.resize(128);
}

CEpoller::~CEpoller()
{
    if (m_epollfd >= 0) {
        close(m_epollfd);
    }
}

void CEpoller::add(int fd, uint64_t data, uint32_t event)
{
    ctrl(fd, data, event, EPOLL_CTL_ADD);
}

void CEpoller::mod(int fd, uint64_t data, uint32_t event)
{
    ctrl(fd, data, event, EPOLL_CTL_MOD);
}

void CEpoller::del(int fd)
{
    ctrl(fd, 0, 0, EPOLL_CTL_DEL);
}

int CEpoller::wait(int ms)
{
    return epoll_wait(m_epollfd, &m_vEpollEvent[0], m_vEpollEvent.size(), ms);
}

void CEpoller::ctrl(int fd, uint64_t data, uint32_t events, int op)
{
    epoll_event ev;
    ev.data.u64 = data;
    ev.events = events;
    if (m_bEdgeTrigger) {
        ev.events |= EPOLLET;
    }

    epoll_ctl(m_epollfd, op, fd, &ev);
}

void CEpoller::setEnableNotify(uint64_t data)
{
    if (!m_notifysock.isValid()) {
        m_notifysock.create();
    }
    add(m_notifysock.getfd(), data, EPOLLIN);
}

void CEpoller::notify(uint64_t data)
{
    if (m_notifysock.isValid()) {
        mod(m_notifysock.getfd(), data, EPOLLIN);
    }
}

}
