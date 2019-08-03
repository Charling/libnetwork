//==========================================================================
/**
* @file	 : SocketServer.h
* @author : Charling(²éÁé)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : ·þÎñ¶Ësocket
*/
//==========================================================================
#ifndef _SOCKETSERVER_H__
#define _SOCKETSERVER_H__

#include "Function.h"
#include "ConnectionManager.h"
#include "INetwork.h"
#include "Connection.h"
#include "CombinData.h"

struct Operation
{
	enum emOperationType
	{	
		OperationType_Null = 0,
		OperationType_Run,
		OperationType_RecvData,
		OperationType_NewConnection,
		OperationType_CloseConnection,
		OperationType_Stop,
		OperationType_SendData,
		OperationType_SendStop,
		OperationType_SendCloseData,

		OperationType_Max
	};
	uint32 type;
	uint32 socketID;
	std::string ip;
	char* data;
	Operation()
	{
		type = OperationType_Null;
		socketID = 0;
		data = nullptr;
		ip = "";
	}
	~Operation()
	{
		type = OperationType_Null;
		socketID = 0; {
			delete data;
			data = nullptr;
		}
		ip = "";
	}
};

struct stClientInfo
{
	uint32 checkTime;
	std::string	ip;

	stClientInfo()
	{
		checkTime = 0;
		ip = "";
	}
};

class SocketServer
	: public ISocketServer
	, private boost::noncopyable
	, public ISocketHander
{
public:
	SocketServer();
	virtual ~SocketServer();

public:
	virtual void sendData(uint32 uSocketId, PacketEncoder& encoder);
	virtual void polling();
	virtual bool run(const char* address, const char* port, ISocketServerDelegate* pCallback/*,bool bVerification*/);
	virtual void disconnect(uint32 uSocketId);
	virtual void stop();
	virtual bool setUserData(uint32 uSocketId, void* data);
	virtual void* getUserData(uint32 uSocketId);
	virtual std::string getRemoteIp(uint32 uSocketId);
	virtual VoidPointerMap& getUserData() { return m_socketsUserData; }
	virtual void setPingBreakInterval(uint32 interval) { m_pingBreakInterval = interval; }

private:
	virtual void sendData(uint32 nSocketId, const void* data, size_t len);
	void handleAccept(const boost::system::error_code& e);		 
	void handleStop();	 
	void runLoop();	 
	void writeData();	 
	void checkOperation(); 
	void ping(uint32 uSocketId);	 
	void clearCombinData(); 
	void recvData(Operation* op);
	void onRecv(uint32 uSocketId, void* data, size_t len);
		 
public:	  
	void addOperation(uint32 type, uint32 socketID = 0, boost::array<char, Connection::MaxLengthBuffer> *buffer_recv_ = nullptr, const std::string& ip = "");

private:
	boost::asio::io_service m_ioService;
	boost::asio::ip::tcp::acceptor m_acceptor;

	ISocketServerDelegate* m_socketServerCallback;
	ConnectionManager m_connectionMgr;
	connection_ptr m_newConnection;
	boost::mutex m_mutexNewConnection;

	uint32 m_preCheckTime;
	bool m_writingData;
	boost::mutex m_mutexWritingData;
	uint32 m_pingBreakInterval;

	std::list< Operation* > m_operations;
	boost::mutex m_mutexOperations;

	std::list< Operation* > m_operationsSend;
	boost::mutex m_mutexOperationsSend;

	std::vector<boost::shared_ptr<boost::thread> > m_threads;
	std::size_t m_threadPoolSize;

	std::map< uint32, stClientInfo > m_socketPings;

	VoidPointerMap m_socketsUserData;
	boost::mutex m_mutexUserData;

	std::map<uint32, CombinData*> m_mapCombinData;
};

extern  "C" 
{
	ISocketServer* CreateSocketServer();
	void DestroySocketServer(ISocketServer* pSocketServer);
}

#endif