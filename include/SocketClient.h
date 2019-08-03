//==========================================================================
/**
* @file	 : SocketClient.h
* @author : Charling(≤È¡È)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : øÕªß∂Àsocket
*/
//==========================================================================
#ifndef _SOCKETCLIENT_H__
#define _SOCKETCLIENT_H__

#include "Function.h"
#include "INetwork.h"
#include "CombinData.h"

struct OperationClient
{
	enum emOperationClientType
	{	
		OperationType_Null,
		OperationClientType_Connect,
		OperationClientType_ReConnect,
		OperationType_RecvData,
		OperationType_CloseConnection,
		OperationType_SendData,
		OperationType_SendCloseData,
		OperationClientType_SendShutDownConnection,

		OperationClientType_Max
	};

	uint32 type;
	char* data;

	OperationClient()
	{
		type = OperationType_Null;
		data = NULL;
	}

	~OperationClient()
	{
		type = OperationType_Null;
		if (data != nullptr) {
			delete data;
			data = nullptr;
		}
	}
};

class SocketClient
	: public ISocketClient
	, private boost::noncopyable
	, public ISocketHander
{
private:
	enum 
	{ 
		MaxLengthBuffer = 1024 * 63,
		MaxPendingSize = 1024,
	};

public:
	SocketClient();
	virtual ~SocketClient();

public:
	virtual bool connect(const char* address, const char* port, ISocketClientDelegate* pCallback, bool bAutoReconnect = false);
	virtual void polling();
	virtual void sendData(PacketEncoder& encoder);
	virtual void close();
	virtual void shutDown();
	virtual void setPingBreakInterval(uint32 interval)	{ m_pingBreakInterval = interval; }

private:
	virtual void sendData(const void* data, size_t len);
	virtual bool isRunning(){ boost::mutex::scoped_lock lock(m_mutexRunning); return m_running; };

	void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred);
	void handleWrite(const boost::system::error_code& e);
	void addOperation(uint32 type, boost::array<char, MaxLengthBuffer>* buffer = nullptr);

	void start();
	void stop();

	void writeData();
	void write(char* data, size_t len);

	void runLoop();
	void clear();

	bool reConnect();
	void popOneByte();

	void onRecv(uint32 uSocketId, void* data, size_t len);
	void recvData(OperationClient* op);

	void checkOperation();

private:
	boost::array<char, MaxLengthBuffer> m_arrBuffer;
	boost::array<char, MaxLengthBuffer> m_arrWriteBuffer;

	boost::asio::io_service m_ioService;
	boost::asio::ip::tcp::socket m_socket;

	ISocketClientDelegate* m_socketClientCallback;

	std::size_t m_bufferDataSize;

	std::list< OperationClient* > m_operations;
	boost::mutex m_mutexOperations;

	std::list< OperationClient* > m_operationsSend;
	boost::mutex m_mutexOperationsSend;

	std::list< char* > m_writes;

	bool m_writing;
	bool m_running;
	boost::mutex m_mutexRunning;

	uint32 m_preRecvTime;
	uint32 m_preCheckTime;

	bool m_autoReconnect;

	std::string m_ip;
	std::string m_port;

	bool m_writingData;
	boost::mutex m_mutexWritingData;

	boost::thread* m_thread;

	uint32 m_pingBreakInterval;

	CombinData* m_combinData;
};

extern  "C" 
{
	ISocketClient* createSocketClient();
	void destroySocketClient(ISocketClient* pSocketClient);
}

#endif // _SOCKETCLIENT_H__