//==========================================================================
/**
* @file	 : connection.h
* @author : Charling(²éÁé)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : ÍøÂçÁ´½ÓÆ÷
*/
//==========================================================================
#ifndef _CONNECTION_H__
#define _CONNECTION_H__

#include "Function.h"

class ConnectionManager;
class SocketServer;
class Connection
	: public boost::enable_shared_from_this<Connection>
	, private boost::noncopyable
{
public:
	enum
	{
		MaxLengthBuffer = 1024 * 63,
		MaxPendingSize = 1024,
	};
public:
	explicit Connection(boost::asio::io_service& io_service, ConnectionManager& manager, SocketServer& callback);
	~Connection();

public:
	boost::asio::ip::tcp::socket& socket();

	void start();
	void stop();
	void write(void* data, size_t len);

	uint32 getSocketId();
	std::string getRemoteIp();
	void	 setUserData(void* data);
	void* getUserData();

private:
	void handleRead(const boost::system::error_code& e, std::size_t bytes);
	void handleWrite(const boost::system::error_code& e);
	void popOneByte();

private:
	boost::asio::ip::tcp::socket m_socket;
	ConnectionManager& m_connectMgr;
	bool m_writing;
	bool m_stopped;
	std::size_t m_bufferDataSize;
	uint32 m_socketId;
	SocketServer& m_socketServer;
	void* m_userData;
	boost::array<char, MaxLengthBuffer> m_buffer;
	boost::array<char, MaxLengthBuffer> m_writeBuffer;

	boost::mutex m_mutexWriting;
	std::list< char* > m_writes;
	boost::mutex m_mutexWrites;
	boost::mutex m_mutexUserData;
};

#endif