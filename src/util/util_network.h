#ifndef _MFW_UTIL_NETWORK_
#define _MFW_UTIL_NETWORK_

#include <stdint.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include "util_noncopyable.h"
using namespace std;

namespace mfw
{

class UtilNet
{
public:
	static bool isIPv4(const string &ip);
	static bool isIPv6(const string &ip);

	static void encodeIPv4(const string &ip, uint16_t port, struct sockaddr_in &addr);
	static void decodeIPv4(const struct sockaddr_in &addr, string &ip, uint16_t &port);
	static void encodeIPv6(const string &ip, uint16_t port, struct sockaddr_in6 &addr);
	static void decodeIPv6(const struct sockaddr_in6 &addr, string &ip, uint16_t &port);
	static void encodeUnix(const string &sPath, struct sockaddr_un &addr);
	static void decodeUnix(const struct sockaddr_un &addr, string &sPath);
	static void encodeAddr(const string &ip, uint16_t port, struct sockaddr_storage &addr);
	static void decodeAddr(const struct sockaddr_storage &addr, string &ip, uint16_t &port);
};

class CEndpoint
{
public:
	enum EndpointType
	{
		EndpointType_Invalid,
		EndpointType_Tcp4,
		EndpointType_Udp4,
		EndpointType_Unix,
	};

	CEndpoint(): m_type(EndpointType_Invalid), m_iPort(0), m_iTimeout(0), m_bWebsocket(false) {}
	CEndpoint(const string &sDesc) { parse(sDesc); }
	CEndpoint(EndpointType type, const string &sHost, uint16_t iPort, uint32_t iTimeout, bool bWebsocket);
	CEndpoint(EndpointType type, const string &sPath, uint32_t iTimeout, bool bWebsocket);

	void reset();
	void parse(const string &sDesc);
	bool parseNoThrow(const string &sDesc);
	bool isValid() const { return m_type != EndpointType_Invalid; }
	bool isTcp() const { return m_type == EndpointType_Tcp4; }
	bool isUdp() const { return m_type == EndpointType_Udp4; }
	bool isUnix() const { return m_type == EndpointType_Unix; }
    bool isWebsocket() const { return m_bWebsocket; }

	const string &getDesc() const { return m_sDesc; }
	const string &getHost() const { return m_sHost; }
	uint16_t getPort() const { return m_iPort; }
	const string &getPath() const { return m_sHost; }
	uint32_t getTimeout() const { return m_iTimeout; }

	bool operator< (const CEndpoint &rhs) const { return getDesc() < rhs.getDesc(); }
	bool operator== (const CEndpoint &rhs) const { return getDesc() == rhs.getDesc(); }

private:
	bool _parse(const string &sDesc);
	void _gendesc();

private:
	EndpointType m_type;
	string		m_sDesc;
	string		m_sHost;
	uint16_t	m_iPort;
	uint32_t	m_iTimeout;
    bool        m_bWebsocket;
};

class CSocket : public noncopyable
{
public:
	enum SockType
	{
		SockType_Tcp4,
		SockType_Udp4,
		SockType_Unix,
	};

	CSocket() : m_sockfd(-1), m_bOwned(false) {}
	~CSocket() { reset(); }

	void init(int sock, bool bOwned = true);
	void create(SockType type = SockType_Tcp4);
	void setOwned(bool bOwned) { m_bOwned = bOwned; }
	int getfd() const { return m_sockfd; }
	bool isValid() const { return m_sockfd >= 0; }

	void close();

	bool connect(const string &ip, uint16_t port);
	bool connect(const string &sPath);
	bool connect(const struct sockaddr *addr, socklen_t len);

	void bind(const string &ip, uint16_t port);
	void bind(const string &sPath);
	void bind(const struct sockaddr *addr, socklen_t len);

	void listen(int backlog = 1024);

	bool accept(CSocket &sock);
	bool accept(CSocket &sock, struct sockaddr *addr, socklen_t *len);

	void setblock(bool bBlock);
	void setTcpNoDelay();
	void setsockopt(int level, int optname, int optval, const char *name = NULL);
	void setsockopt(int level, int optname, const void *optval, socklen_t optlen, const char *name = NULL);

	int getsockerror();
	void getsockopt(int level, int optname, int &optval, const char *name = NULL);
	void getsockopt(int level, int optname, void *optval, socklen_t *optlen, const char *name = NULL);

	int recv(void *buf, uint32_t len);
	int send(const void *buf, uint32_t len);

	int recvfrom(void *buf, uint32_t len, string &ip, uint16_t &port);
	int recvfrom(void *buf, uint32_t len, struct sockaddr *addr, socklen_t *socklen);

	int sendto(const void *buf, uint32_t len, const string &ip, uint16_t port);
	int sendto(const void *buf, uint32_t len, const struct sockaddr *addr, socklen_t socklen);

private:
	void reset();

private:
	int		m_sockfd;
	bool	m_bOwned;
};

class CEpoller : public noncopyable
{
public:
	explicit CEpoller(bool bEdgeTrigger = true);
	~CEpoller();

	void add(int fd, uint64_t data, uint32_t event);
	void mod(int fd, uint64_t data, uint32_t event);
	void del(int fd);
	int wait(int ms);
	epoll_event &get(int i) { return m_vEpollEvent[i]; }

	void setEnableNotify(uint64_t data);
	void notify(uint64_t data);

private:
	void ctrl(int fd, uint64_t data, uint32_t events, int op);

private:
	bool m_bEdgeTrigger;
	int	m_epollfd;
	vector<epoll_event> m_vEpollEvent;
	CSocket m_notifysock;
};

}

#endif
