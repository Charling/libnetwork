//==========================================================================
/**
* @file	 : ConnectionManager.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : 网络链接器管理器
*/
//==========================================================================
#ifndef _CONNECTIONMANAGER_H__
#define _CONNECTIONMANAGER_H__

#include "Function.h"

class Connection;
typedef boost::shared_ptr<Connection> connection_ptr;

class ConnectionManager
	: private boost::noncopyable
{
public:
	void startConnection(connection_ptr c);
	void stopConnection(connection_ptr c);
	void stopAll();
	connection_ptr getConnectPtr(uint32 socketId);
	uint32 generateSocketId();
	void recycleSocketId(uint32 socketId);	 
	bool write(uint32 socketId, void* pData, size_t nDataLen);	 
	bool setUserData(uint32 uSocketId, void* pUserData);
	void* getUserData(uint32 uSocketId);

private:
	std::map<uint32, connection_ptr> m_connections;
	std::set<uint32> m_socketIds;
	boost::mutex m_mutexConnections;
	boost::mutex m_mutexSocketIds;
};

#endif