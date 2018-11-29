#include "service/Transport.h"
#include "service/ApplicationConfig.h"
#include "service/ConnectorImp.h"
#include "service/AdapterProxy.h"
#include "util/util_time.h"
#include <errno.h>

namespace mfw
{

Transport::Transport(AdapterProxy *pAdapterProxy)
	: m_pAdapterProxy(pAdapterProxy), m_ep(pAdapterProxy->getEndpoint()), m_connStatus(eUnconnected), m_iConnectTimeoutTime(0)
{
}

Transport::~Transport()
{
    close();
}

void Transport::setConnected()
{
    m_connStatus = eConnected;
    m_pAdapterProxy->setConnTimeout(false);
    m_pAdapterProxy->setConnExc(false);
}

void Transport::checkTimeout()
{
    if (isConnecting() && UtilTime::getNowMS() > m_iConnectTimeoutTime)
    {
        MFW_ERROR("connect to " << m_pAdapterProxy->getFullDesc() << " timeout");
        m_pAdapterProxy->setConnTimeout(true);
        close();
    }
}

void Transport::reconnect()
{
    close();
    connect();
}

void Transport::connect()
{
    if (isValid())
    {
    	return;
    }

    if (eConnecting == m_connStatus || eConnected == m_connStatus)
    {
        return;
    }

	if (m_ep.isUdp())
	{
		m_sock.create(CSocket::SockType_Udp4);
		m_sock.setblock(false);
        m_connStatus = eConnected;
	}
	else
	{
		if (m_ep.isTcp())
		{
			m_sock.create(CSocket::SockType_Tcp4);
			m_sock.setTcpNoDelay();
		}
		else
		{
			m_sock.create(CSocket::SockType_Unix);
		}
		m_sock.setblock(false);

		bool bConnected = m_ep.isTcp() ? m_sock.connect(m_ep.getHost(), m_ep.getPort()) : m_sock.connect(m_ep.getPath());
        if (bConnected)
        {
            setConnected();
        }
        else
        {
            m_connStatus = eConnecting;
            m_iConnectTimeoutTime = UtilTime::getNowMS() + m_pAdapterProxy->getObjectProxy()->getConnectTimeout();
        }
	}

	MFW_DEBUG((m_connStatus == eConnected ? "connected" : "connecting") << " to " << m_pAdapterProxy->getFullDesc() << ", fd: " << m_sock.getfd());
    m_pAdapterProxy->getObjectProxy()->getConnectorImp()->addFd(m_sock.getfd(), this);
}

void Transport::close()
{
    if (!isValid())
    {
    	return;
    }

	m_pAdapterProxy->getObjectProxy()->getConnectorImp()->delFd(m_sock.getfd());
	m_sock.close();

    m_connStatus = eUnconnected;
    m_sSendBuffer.clear();
    m_sRecvBuffer.clear();

    MFW_DEBUG("close connection to " << m_pAdapterProxy->getFullDesc());
}

int Transport::doRequest()
{
    if (!isValid())
    {
    	return -1;
    }

    if (!m_sSendBuffer.empty())
    {
		int ret = send(m_sSendBuffer.c_str(), m_sSendBuffer.size());
        if (ret < 0)
        {
            return ret;
        }
		if (ret > 0)
		{
			if ((size_t)ret == m_sSendBuffer.size())
            {
                m_sSendBuffer.clear();
            }
            else
			{
				m_sSendBuffer.erase(m_sSendBuffer.begin(), m_sSendBuffer.begin() + ret);
				return 0;
			}
		}
    }

    m_pAdapterProxy->doInvoke();
    return 0;
}

bool Transport::trySendRequest(const char *buf, uint32_t len)
{
    if (!isConnected())
    {
        return false;
    }

    if (len == 0)
    {
        return true;
    }

    if (!m_sSendBuffer.empty())
    {
        return false;
    }

    int ret = send(buf, len);
    if (ret < 0)
    {
        return false;
    }
    if ((uint32_t)ret < len)
    {
        m_sSendBuffer.assign(buf + ret, len - ret);
    }
    return true;
}

int TcpTransport::send(const void *buf, uint32_t len)
{
    if (!isConnected())
    {
        return -1;
    }

    int ret = m_sock.send(buf, len);
	if (ret < 0 && errno != EAGAIN)
    {
		MFW_ERROR("tcp send fail: " << strerror(errno) << ", adapter: " << m_pAdapterProxy->getFullDesc());
        close();
    }
    return ret;
}

int TcpTransport::recv(void *buf, uint32_t len)
{
	if (!isConnected())
	{
        return -1;
	}

    int ret = m_sock.recv(buf, len);
    if (ret == 0)
    {
    	MFW_DEBUG("closed by server, adapter: " << m_pAdapterProxy->getFullDesc());
    	close();
    }
    else if (ret < 0 && errno != EAGAIN)
    {
		MFW_ERROR("tcp recv fail: " << strerror(errno) << ", adapter: " << m_pAdapterProxy->getFullDesc());
        close();
    }
    return ret;
}

int TcpTransport::doResponse(list<ResponsePacket> &done)
{
    if (!isValid())
    {
    	return -1;
    }

    done.clear();
    char buf[TCP_RECV_BUFFER_SIZE];
    while (true)
    {
    	int ret = recv(buf, sizeof(buf));
    	if (ret <= 0)
    	{
    		break;
    	}
    	m_sRecvBuffer.append(buf, ret);
    	if (ret < (int)sizeof(buf))
    	{
    		break;
    	}
    }

    if (!m_sRecvBuffer.empty())
    {
        try
        {
            size_t pos = m_pAdapterProxy->getObjectProxy()->getClientSideProtocol().responseFunc(m_sRecvBuffer.c_str(), m_sRecvBuffer.length(), done);
            if (pos > 0)
            {
                m_sRecvBuffer.erase(m_sRecvBuffer.begin(), m_sRecvBuffer.begin() + pos);

                if (m_sRecvBuffer.capacity() - m_sRecvBuffer.length() > TCP_RECV_MAX_RESERVE_SIZE)
                {
                	m_sRecvBuffer.reserve(std::max(m_sRecvBuffer.length(), (size_t)TCP_RECV_MIN_RESERVE_SIZE));
                }
            }
        }
        catch (std::exception &e)
        {
            MFW_ERROR("tcp recv decode fail: " << e.what() << ", adapter: " << m_pAdapterProxy->getFullDesc());
            close();
            done.clear();
        }
    }

    return done.empty() ? 0 : 1;
}

int UdpTransport::send(const void *buf, uint32_t len)
{
    if (!isValid())
    {
    	return -1;
    }

    int ret= m_sock.sendto(buf, len, m_ep.getHost(), m_ep.getPort());
    if (ret < 0 && errno != EAGAIN)
    {
    	MFW_ERROR("udp send fail: " << strerror(errno) << ", adapter: " << m_pAdapterProxy->getFullDesc());
    	close();
    	return ret;
    }
    if (ret >= 0 && ret != (int)len)
    {
    	MFW_ERROR("udp send partial: " << ret << " of " << len << ", adapter: " << m_pAdapterProxy->getFullDesc());
    	ret = len;
    }
    return ret;
}

int UdpTransport::recv(void *buf, uint32_t len)
{
    if (!isValid())
    {
    	return -1;
    }

    int ret = m_sock.recvfrom(buf, len, NULL, NULL);
    if (ret < 0  && errno != EAGAIN)
    {
    	MFW_ERROR("udp recv fail: " << strerror(errno) << ", adapter: " << m_pAdapterProxy->getFullDesc());
		close();
    }
    return ret;
}

int UdpTransport::doResponse(list<ResponsePacket> &done)
{
    if (!isValid())
    {
    	return -1;
    }

    done.clear();
    char buf[UDP_RECV_BUFFER_SIZE];
    while (true)
    {
    	int len = recv(buf, sizeof(buf));
    	if (len <= 0)
    	{
    		break;
    	}

        try
        {
            m_pAdapterProxy->getObjectProxy()->getClientSideProtocol().responseFunc(buf, len, done);
        }
        catch (std::exception &e)
        {
            MFW_ERROR("udp recv decode fail: " << e.what() << ", adapter: " << m_pAdapterProxy->getFullDesc());
            done.clear();
        }
    }

    return done.empty() ? 0 : 1;
}

}
