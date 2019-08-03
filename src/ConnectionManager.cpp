#include "Connection.h"
#include "ConnectionManager.h"
#include "Log.h"


void ConnectionManager::startConnection(connection_ptr c)
{
	{
		boost::mutex::scoped_lock lock(m_mutexConnections);
		m_connections[c->getSocketId()] = c;
		c->start();
	}

	boost::system::error_code ignored_ec;
	boost::asio::ip::tcp::endpoint endpoint = c->socket().remote_endpoint(ignored_ec);
	std::string str_ip = endpoint.address().to_string(ignored_ec);
}

void ConnectionManager::stopConnection(connection_ptr c)
{
	if (c == NULL)
		return;

	boost::mutex::scoped_lock lock(m_mutexConnections);
	auto iter = m_socketIds.find(c->getSocketId());
	if (iter == m_socketIds.end())
		return;

	boost::system::error_code ignored_ec;
	boost::asio::ip::tcp::endpoint endpoint = c->socket().remote_endpoint(ignored_ec);
	std::string str_ip = endpoint.address().to_string(ignored_ec);

	c->stop();
	m_socketIds.erase(iter);
}

connection_ptr ConnectionManager::getConnectPtr(uint32 socketId)
{
	boost::mutex::scoped_lock lock(m_mutexConnections);
	auto iter = m_connections.find(socketId);

	if (iter == m_connections.end())
		return connection_ptr();

	return iter->second;

}

bool ConnectionManager::setUserData(uint32 uSocketId, void* pUserData)
{
	connection_ptr connection_ = connection_ptr();
	boost::mutex::scoped_lock lock(m_mutexConnections);
	auto iter = m_connections.find(uSocketId);
	if (iter != m_connections.end())
		connection_ = iter->second;

	if (connection_ != connection_ptr())
	{
		connection_->setUserData(pUserData);
		return true;
	}
	return false;
}

bool ConnectionManager::write(uint32 socketId, void* pData, size_t nDataLen)
{
	connection_ptr connection_ = connection_ptr();
	boost::mutex::scoped_lock lock(m_mutexConnections);
	auto iter = m_connections.find(socketId);
	if (iter != m_connections.end())
		connection_ = iter->second;

	if (connection_ != connection_ptr())
	{
		connection_->write(pData, nDataLen);
		return true;
	}
	return false;
}

void* ConnectionManager::getUserData(uint32 uSocketId)
{
	connection_ptr connection_ = connection_ptr();
	boost::mutex::scoped_lock lock(m_mutexConnections);
	auto iter = m_connections.find(uSocketId);
	if (iter != m_connections.end())
		connection_ = iter->second;

	if (connection_ != connection_ptr())
		return connection_->getUserData();
	return NULL;
}

void ConnectionManager::stopAll()
{
	boost::mutex::scoped_lock lock(m_mutexConnections);
	auto iter = m_connections.begin();
	auto iterend = m_connections.end();
	for (; iter != iterend; ++iter)
		iter->second->stop();

	m_socketIds.clear();
}

uint32 ConnectionManager::generateSocketId()
{
	boost::mutex::scoped_lock lock(m_mutexSocketIds);
	static uint32 current_socket_id = 65536;

	while (1)
	{
		++current_socket_id;
		if (current_socket_id == 0)
			continue;

		if (std::find(m_socketIds.begin(), m_socketIds.end(), current_socket_id) == m_socketIds.end())
		{
			m_socketIds.insert(current_socket_id);
			return current_socket_id;
		}
	}
}

void ConnectionManager::recycleSocketId(uint32 socketId)
{
	boost::mutex::scoped_lock lock(m_mutexSocketIds);
	m_socketIds.erase(socketId);
}