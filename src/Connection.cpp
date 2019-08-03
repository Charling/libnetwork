#include "Log.h"
#include "Connection.h"
#include "ConnectionManager.h"
#include "SocketServer.h"

Connection::Connection(boost::asio::io_service& io_service, ConnectionManager& manager, SocketServer& callback)
	: m_socket(io_service)
	, m_connectMgr(manager)
	, m_writing(false)
	, m_stopped(false)
	, m_bufferDataSize(0)
	, m_socketId(m_connectMgr.generateSocketId())
	, m_socketServer(callback)
	, m_userData(nullptr)
{
}

Connection::~Connection()
{
	{
		boost::mutex::scoped_lock lock(m_mutexWrites);
		do
		{
			auto iter = m_writes.begin();
			if (iter == m_writes.end())
				break;
			char* p = *iter;
			m_writes.pop_front();
			delete p;

		} while (1);
	}

	m_connectMgr.recycleSocketId(m_socketId);
}

boost::asio::ip::tcp::socket& Connection::socket()
{
	return m_socket;
}

void Connection::start()
{
	m_socket.async_read_some(boost::asio::buffer(m_buffer)
		, boost::bind(&Connection::handleRead
		, shared_from_this()
		, boost::asio::placeholders::error
		, boost::asio::placeholders::bytes_transferred));

	m_socketServer.addOperation(Operation::OperationType_NewConnection, getSocketId(), &m_buffer, getRemoteIp());
}

void Connection::stop()
{
	if (m_socket.is_open() && !m_stopped)
	{
		m_socketServer.addOperation(Operation::OperationType_CloseConnection, getSocketId(), nullptr, getRemoteIp());
		m_stopped = true;
		boost::system::error_code ignored_ec;
		boost::asio::socket_base::linger linger_op(true, 0);
		m_socket.set_option(linger_op);
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		m_socket.close(ignored_ec);
	}
}

void Connection::write(void* data, size_t len)
{
	{
		if (!m_socket.is_open())
		{
			delete data;
			return;
		}
	}

	if (len > MaxLengthBuffer)
	{
		LOGERROR("%s len > MaxLengthBuffer:len:%d ", __FUNCTION__, len);
		delete data;
		return;
	}

	if (m_writing)
	{
		boost::mutex::scoped_lock lock(m_mutexWrites);
		if (m_writes.size() >= MaxPendingSize)
		{
			delete data;
			return;
		}
		m_writes.push_back((char*)data);
		return;
	}

	{
		boost::mutex::scoped_lock lock(m_mutexWriting);
		m_writing = true;
	}

	size_t nSendLen = 0;

	memcpy(m_writeBuffer.data(), data, len);
	*((uint32*)m_writeBuffer.data()) = (uint32)len - sizeof(uint32);
	nSendLen += len;

	boost::asio::async_write(m_socket,
		boost::asio::buffer(m_writeBuffer.data(), nSendLen),
		boost::bind(&Connection::handleWrite, shared_from_this(), boost::asio::placeholders::error));

	delete data;

	return;
}

void Connection::popOneByte()
{
	if (m_bufferDataSize > 0)
	{
		m_bufferDataSize -= 1;
		if (m_bufferDataSize > 0) {
			memmove(m_buffer.data(), m_buffer.data() + 1, m_bufferDataSize);
		}
	}
}

void Connection::handleRead(const boost::system::error_code& e, std::size_t bytes)
{
	if (!e)
	{
		m_bufferDataSize += bytes;

		uint32 missByteCount = 0;
		while (1)
		{
			if (m_bufferDataSize <= sizeof(uint32))
				break;

			uint32 nNeedRecvLen = *((uint32*)m_buffer.data()) + sizeof(uint32);

			if (nNeedRecvLen < sizeof(uint32) || nNeedRecvLen > MaxLengthBuffer)
			{//收到不良包,直接断开链接好了
				//	LOGERROR("Connection::HandleRead: nNeedRecvLen < sizeof(uint32) || nNeedRecvLen > MaxLengthBuffer :nNeedRecvLen = %d ", nNeedRecvLen);
				LOGERROR("warning warning warning !!! get hacker attack ? network package size = %d, ip = %s.", nNeedRecvLen, getRemoteIp().c_str());

				popOneByte();
				missByteCount++;
				stop();
				return;
			}

			if (nNeedRecvLen == sizeof(uint32))
			{
				LOGINFO("Connection::HandleRead: nNeedRecvLen = sizeof(uint32) :nNeedRecvLen = %d ", nNeedRecvLen);

				popOneByte();
				missByteCount++;
				continue;
			}

			if (nNeedRecvLen > m_bufferDataSize)
			{
				//LOGINFO("Connection::HandleRead: nNeedRecvLen > m_bufferDataSize :%d,m_bufferDataSize :%d ",nNeedRecvLen,m_bufferDataSize);
				break;
			}

			m_socketServer.addOperation(Operation::OperationType_RecvData, getSocketId(), &m_buffer);
			m_bufferDataSize -= nNeedRecvLen;
			if (m_bufferDataSize > 0)
			{
				memmove(m_buffer.data(), m_buffer.data() + nNeedRecvLen, m_bufferDataSize);
			}

		}

		int32 nextReadLen = 0;
		if (m_bufferDataSize < sizeof(uint32))
		{
			nextReadLen = sizeof(uint32) - (uint32)m_bufferDataSize;
		}
		else
		{
			do
			{
				nextReadLen = *((uint32*)m_buffer.data()) + sizeof(uint32) - (uint32)m_bufferDataSize;
				if (nextReadLen > 0 && (nextReadLen + m_bufferDataSize) <= MaxLengthBuffer)
				{
					//LOGERROR("%s: nextReadLen < 0 || (nextReadLen + m_bufferDataSize) > MaxLengthBuffer,nextReadLen = %d ",__FUNCTION__,nextReadLen);("Connection::HandleRead: nextReadLen < 0 || (nextReadLen + m_bufferDataSize) > MaxLengthBuffer,nextReadLen = %d ",nextReadLen);
					break;
				}
				popOneByte();
				missByteCount++;
				if (m_bufferDataSize < sizeof(uint32))
				{
					nextReadLen = sizeof(uint32) - (uint32)m_bufferDataSize;
					break;
				}

			} while (1);

		}

		if (missByteCount > 0)
			LOGINFO("Connection::HandleRead: missByteCount = %d ", missByteCount);

		m_socket.async_read_some(boost::asio::buffer(m_buffer.data() + m_bufferDataSize, nextReadLen),//MaxLengthBuffer - m_bufferDataSize),
			boost::bind(&Connection::handleRead, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));

	}
	else
	{
		if (e != boost::asio::error::eof)
		{//除了正常的断线之外,其他需要log
			LOGERROR("Connection::HandleRead e'value=%d, e'string=%s, socket id=%u, ip=%s.", e.value(), e.message().c_str(), m_socketId, getRemoteIp().c_str());
		}

		if (e == boost::asio::error::operation_aborted)
		{
			LOGERROR("Err:Connection::HandleRead e == boost::asio::error::operation_aborted ");
		}
		else
		{
			m_connectMgr.stopConnection(shared_from_this());
		}
	}
}

void Connection::handleWrite(const boost::system::error_code& e)
{
	{
		boost::mutex::scoped_lock lock(m_mutexWriting);
		m_writing = false;
	}

	if (!e)
	{
		char* p = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexWrites);
			if (m_writes.size() > 0)
			{
				stl_list< char* >::iterator iter = m_writes.begin();
				p = *iter;
				m_writes.pop_front();
			}
		}

		if (p != nullptr)
		{
			write(p, *((uint32*)p));
		}
	}
	else
	{
		if (e != boost::asio::error::eof)
		{//除了正常的断线之外,其他需要log
			LOGERROR("Connection::HandleWrite e'value=%d, e'string=%s, socket id=%u, ip=%s.", e.value(), e.message().c_str(), m_socketId, getRemoteIp().c_str());
		}

		if (e == boost::asio::error::operation_aborted)
			LOGERROR("Err:%s e == boost::asio::error::operation_aborted ", __FUNCTION__);
		else
		{
			m_connectMgr.stopConnection(shared_from_this());
		}
	}

}

uint32 Connection::getSocketId()
{
	return m_socketId;
}

std::string Connection::getRemoteIp()
{
	boost::system::error_code ignored_ec;
	boost::asio::ip::tcp::endpoint endpoint = socket().remote_endpoint(ignored_ec);
	return endpoint.address().to_string(ignored_ec);
}

void Connection::setUserData(void* data)
{
	boost::mutex::scoped_lock lock(m_mutexUserData);
	m_userData = data;
}

void* Connection::getUserData()
{
	boost::mutex::scoped_lock lock(m_mutexUserData);
	return m_userData;
}