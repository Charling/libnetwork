//==========================================================================
/**
* @file	 : SocketServer.cpp
* @author : Charling(查灵)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : 服务端socket
*/
//==========================================================================
#include "SocketServer.h"
#include "MemoryManager.h"
#include "Connection.h"
#include "Log.h"
#include "SocketClient.h"
using namespace Base;

SocketServer::SocketServer()
	: m_ioService()
	, m_acceptor(m_ioService)
	, m_socketServerCallback(nullptr)
	, m_connectionMgr()
	, m_newConnection()
	, m_preCheckTime(0)
	, m_writingData(false)
	, m_pingBreakInterval(0)
{
	m_mapCombinData.clear();
}

SocketServer::~SocketServer()
{
	do
	{
		Operation* op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperations);
			if (m_operations.size() > 0)
			{
				op = *(m_operations.begin());
				m_operations.pop_front();
			}
		}

		if (op == nullptr) break;
		delete op;
		op = nullptr;
	} while (1);

	do
	{
		Operation* op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperationsSend);
			if (m_operationsSend.size() > 0)
			{
				op = *(m_operationsSend.begin());
				m_operationsSend.pop_front();
			}
		}
		if (op == nullptr)
			break;
		delete op;
		op = nullptr;
	} while (1);

}

void SocketServer::ping(uint32 uSocketId)
{
	static PacketEncoder encoder;
	encoder.clear();
	encoder.write(int32(0));
	encoder.write(int32(0));
	encoder.write(int32(m_pingBreakInterval));
	sendData(uSocketId, encoder);
}

void SocketServer::sendData(uint32 uSocketId, PacketEncoder& encoder)
{
	//发送的数据长度超过了默认的组包算法最大长度
	//合并发送的思路:
	//1.当发送的数据包大于预先设定的值时,系统自动将这个包拆开分成多个包进行发送
	//2.当接受方发现收到一个MAX_NET_PACKAGE_SIZE大小的数据包时，就表示需要和后面那个数据包合并
	//3.需要特别处理的是，如果发送的数据包正好是MAX_NET_PACKAGE_SIZE字节,则在后面追加4个特殊字节(0xBADDBADD),这种情况出现的概率非常低
	int32 size = encoder.getDataSize();

	if (size < MAX_NET_PACKAGE_SIZE)
	{//考虑到客户端每个包不可能超过16k, 所以正常包不能受影响
		if (size == 0)
			return;

		void* p = const_cast<void*>(encoder.getData());

		sendData(uSocketId, (char*)p + sizeof(uint32), size - sizeof(uint32));
	}
	else
	{
		void* p = const_cast<void*>(encoder.getData());
		char* pRealData = (char*)p + sizeof(uint32);

		//此处减去记录长度的4个字节
		size -= 4;
		int32 totalSize = 0;
		while (size > 0)
		{
			int32 needSize = MIN(MAX_NET_PACKAGE_SIZE - 1, size);

			char* value = new char[needSize];
			memcpy(value, pRealData, needSize);
			sendData(uSocketId, value, needSize);

			pRealData += needSize;
			size -= needSize;
			totalSize += needSize;
	
			delete value;

			if (size == 0 && needSize == MAX_NET_PACKAGE_SIZE - 1)
			{//没办法，只能特殊处理了,通知此包已经结尾
				uint32* nPadding = new uint32[sizeof(uint32)];
				*((uint32*)(nPadding)) = COMBIN_SEND_PADDING;
				sendData(uSocketId, (char*)nPadding, sizeof(uint32));
				delete nPadding;
			}
		}
	}
}

void SocketServer::checkOperation()
{
	if (m_socketServerCallback == nullptr)
		return;

	if (getTickCount() - m_preCheckTime > m_pingBreakInterval / 10)
	{
		auto iter = m_socketPings.begin();
		auto iterend = m_socketPings.end();
		for (; iter != iterend; ++iter)
		{
			if (getTickCount() - iter->second.checkTime > m_pingBreakInterval)
			{
				//LOGINFO("CloseConnection:%d,ping_break_interval:%d ",iter->first,m_pingBreakInterval);
				disconnect(iter->first);
			}
			else if (getTickCount() - iter->second.checkTime > m_pingBreakInterval / 10)
			{
				if (m_socketServerCallback->canPing())
					m_socketServerCallback->onPing(iter->first);
				else
					ping(iter->first);
			}
		}
		m_preCheckTime = getTickCount();
	}

}

void SocketServer::polling()
{
	if (m_socketServerCallback == nullptr)
		return;

	//m_socketServerCallback->OnUpdate();

	{
		boost::mutex::scoped_lock lock(m_mutexWritingData);
		if (!m_writingData)
		{
			m_ioService.post(boost::bind(&SocketServer::writeData, this));
			m_writingData = true;
		}
	}

	uint32 count = 0;
	uint32 operations_size = 0;
	do
	{
		checkOperation();

		Operation* op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperations);
			if (m_operations.size() > 0)
			{
				op = *(m_operations.begin());
				m_operations.pop_front();
				if (operations_size == 0)
					operations_size = (uint32)m_operations.size();
			}
		}

		if (op == nullptr) 
			break;

		switch (op->type)
		{
			case Operation::OperationType_Run:
			{
				m_socketPings.clear();
				clearCombinData();
			} break;
			case Operation::OperationType_NewConnection:
			{
				m_socketServerCallback->onConnect(op->socketID, op->ip);
				stClientInfo tmp;
				tmp.checkTime = getTickCount();
				tmp.ip = op->ip;
				m_socketPings[op->socketID] = tmp;
				CombinData* pCombinData = new CombinData();
				IF_NOT_RETURN(pCombinData);
				m_mapCombinData[op->socketID] = pCombinData;
			} break;
			case Operation::OperationType_CloseConnection:
			{
				m_socketServerCallback->onDisconnect(op->socketID, op->ip);
				auto iter = m_socketPings.find(op->socketID);
				if (iter != m_socketPings.end())
					m_socketPings.erase(iter);

				auto it = m_mapCombinData.find(op->socketID);
				if (it != m_mapCombinData.end())
				{
					CombinData* pCombinData = it->second;
					IF_NOT_RETURN(pCombinData);
					delete pCombinData;
					m_mapCombinData.erase(it);
				}
				break;
			}
			case Operation::OperationType_RecvData:
			{
				recvData(op);
				delete op->data;
				op->data = nullptr;
				//记录client的消息反馈
				m_socketPings[op->socketID].checkTime = getTickCount();

			} break;
			case Operation::OperationType_Stop:
			{
				m_socketPings.clear();
				clearCombinData();
			} break;
			default:
				break;
		}
		delete op;
		count++;
		if (count >= operations_size) {
			break;
		}
	} while (1);
}

void SocketServer::clearCombinData()
{
	for (auto it = m_mapCombinData.begin(); it != m_mapCombinData.end(); ++it)
	{
		CombinData* pCombinData = it->second;
		IF_NOT_RETURN(pCombinData);
		delete pCombinData;
	}
	m_mapCombinData.clear();
}

void SocketServer::onRecv(uint32 uSocketId, void* data, size_t len)
{
	if (uSocketId == INVALID_CLIENT_SOCKETID)
	{
		LOGERROR("OnRecvData uScoketId == INVALID_CLIENT_SOCKETID");
		return;
	}

	void* pUserData = getUserData(uSocketId);

	if (len < sizeof(MessageHeader))
	{
		LOGERROR("len >= sizeof(MessageHeader)");
		return;
	}

	PacketDecoder decoder;
	if (!decoder.setData(reinterpret_cast<const char*>(data), (int32)(len + 4)))
	{
		LOGERROR("!decoder.setData(reinterpret_cast<const char*>(data), len + 4)");
		return;
	}

	if (decoder.getCount() < 2)
	{
		LOGERROR("decoder.getCount() < 2");
		return;
	}

	MessageHeader hdr;
	if (!decoder.read(0, hdr.msgType) || !decoder.read(1, hdr.msgId))
	{
		LOGERROR("!decoder.read(0, hdr.msgType) || !decoder.read(1, hdr.msgId)");
		return;
	}

	if (hdr.msgId < 0 || hdr.msgId >= 65536)
	{
		LOGERROR("hdr.msgId < 0 || hdr.msgId >= 65536,id:%d", hdr.msgId);
		return;
	}

	PacketDecoder* pDecoder = &decoder;

	m_socketServerCallback->onRecvData(uSocketId, hdr, pDecoder, pUserData);
}

void SocketServer::recvData(Operation* op)
{
	IF_NOT_RETURN(op);

	uint32 uSocketId = op->socketID;
	void* data = op->data;
	size_t len = *((uint32*)op->data);

	if (uSocketId == INVALID_CLIENT_SOCKETID)
	{
		LOGERROR("OnRecvData uScoketId == INVALID_CLIENT_SOCKETID");
		return;
	}

	//发送的数据长度超过了默认的组包算法最大长度
	//合并发送的思路:
	//1.当发送的数据包大于预先设定的值时,系统自动将这个包拆开分成多个包进行发送
	//2.当接受方发现收到一个MAX_NET_PACKAGE_SIZE大小的数据包时，就表示需要和后面那个数据包合并
	//3.需要特别处理的是，如果发送的数据包正好是MAX_NET_PACKAGE_SIZE字节,则在后面追加4个特殊字节(0xBADDBADD),这种情况出现的概率非常低

	CombinData* pCombinData = nullptr;
	auto it = m_mapCombinData.find(uSocketId);
	if (it == m_mapCombinData.end())
	{//错误，不可能发生
		IF_NOT_RETURN("it == m_mapCombinData.end()");
	}
	pCombinData = it->second;
	IF_NOT_RETURN(pCombinData);

	if (pCombinData->getCommbinLen() > 0)
	{//目前处于组合包
		char* pRealData = reinterpret_cast<char*>(data)+4;

		if (len == 4 && *((uint32*)(pRealData)) == COMBIN_SEND_PADDING)
		{//终结包,此概率极低
			pCombinData->endPackage(uSocketId, this);
			return;
		}

		//组建包
		pCombinData->makePackage(pRealData, len);

		if (len != MAX_NET_PACKAGE_SIZE - 1)
		{//终结包
			pCombinData->endPackage(uSocketId, this);
		}

		return;
	}

	if (len == MAX_NET_PACKAGE_SIZE - 1)
	{//收到MAX_NET_PACKAGE_SIZE的包就表示后续还有包
		//首包,包括长度4个字节
		pCombinData->fisrtPackage(data, len + 4);

		return;
	}

	onRecv(uSocketId, data, len);
}

void SocketServer::sendData(uint32 nSocketId, const void* data, size_t len)
{
	if (data == nullptr || len == 0)
	{
		//LOGINFO("Error:%s data == nullptr || len == 0 len = %d ",__FUNCTION__,len);
		return;
	}

	if (len + sizeof(uint32) > Connection::MaxLengthBuffer)
	{
		LOGERROR("%s len + sizeof(uint32) > Connection::MaxLengthBuffer:%d ", __FUNCTION__, len);
		return;
	}

	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		if (m_operationsSend.size() >= Connection::MaxPendingSize * 1024)
		{
			//LOGINFO("%s:m_operationsSend.size() >= Connection::MaxPendingSize * 1024:%d ",__FUNCTION__,Connection::MaxPendingSize * 1024);
			return;
		}
	}

	Operation* op = new Operation();
	if (op == nullptr) return;
	op->type = Operation::OperationType_SendData;
	op->socketID = nSocketId;
	op->data = new char[len + sizeof(uint32)];
	if (op->data == nullptr)
	{
		//LOGINFO("%s:op->data = %p ",__FUNCTION__,op->data);
		return;
	}
	*((uint32*)op->data) = (uint32)(len + sizeof(uint32));
	memcpy(op->data + sizeof(uint32), data, len);

	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		m_operationsSend.push_back(op);

		boost::mutex::scoped_lock locker(m_mutexWritingData);
		if (!m_writingData)
		{
			m_ioService.post(boost::bind(&SocketServer::writeData, this));
			m_writingData = true;
		}
	}
}

void SocketServer::writeData()
{
	uint32 count = 0;
	uint32 size_operations = 0;
	do
	{
		Operation* op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperationsSend);
			if (m_operationsSend.size() > 0)
			{
				op = *(m_operationsSend.begin());
				m_operationsSend.pop_front();
				if (size_operations == 0)
					size_operations = (uint32)m_operationsSend.size();
			}
		}
		if (op == nullptr)
			break;
		switch (op->type)
		{
			case Operation::OperationType_SendData:
			{
				if (m_connectionMgr.write(op->socketID, op->data, *((uint32*)op->data)))
					op->data = nullptr;
				break;
			}
			case Operation::OperationType_SendStop:
			{
				m_ioService.post(boost::bind(&SocketServer::handleStop, this));
				break;
			}
			case Operation::OperationType_SendCloseData:
			{
				m_connectionMgr.stopConnection(m_connectionMgr.getConnectPtr(op->socketID));
				break;
			}
			default:
				break;
		}
		delete op;
		op = nullptr;
		count++;
		if (count >= size_operations)
		{
			break;
		}
	} while (1);

	{
		boost::mutex::scoped_lock lock(m_mutexWritingData);
		m_writingData = false;
	}
}

void SocketServer::runLoop()
{
	while (1)
	{
		try
		{
			m_ioService.run();
		}
		catch (...)
		{
			//LOGINFO("%s catch!!!!!!!! ",__FUNCTION__);
		}
	}

}

bool SocketServer::run(const char* address, const char* port, ISocketServerDelegate* pCallback/*,bool bVerification*/)
{

	if (pCallback == nullptr)
	{
		//LOGINFO("%s pCallback = %p ",__FUNCTION__,pCallback);
		return false;
	}
	try
	{
		//	pCallback->OnRun();
		m_socketServerCallback = pCallback;
		m_newConnection = connection_ptr(new Connection(m_ioService, m_connectionMgr, *this));
		boost::asio::ip::tcp::resolver resolver(m_ioService);
		boost::asio::ip::tcp::resolver::query query(address, port);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
		m_acceptor.open(endpoint.protocol());
		m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		m_acceptor.bind(endpoint);
		m_acceptor.listen();
		m_acceptor.async_accept(m_newConnection->socket(),
			boost::bind(&SocketServer::handleAccept, this,
			boost::asio::placeholders::error));

		m_threadPoolSize = 1;

		for (std::size_t i = 0; i < m_threadPoolSize; ++i)
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(
				boost::bind(&SocketServer::runLoop, this)));
			m_threads.push_back(thread);
		}
	}
	catch (std::exception& e)
	{
		LOGERROR("SocketServer::Run Exception:%s,ip:%s,port:%s", e.what(), address, port);
		return false;
	}
	return true;
}

void SocketServer::stop()
{
	Operation* op = new Operation();
	if (op == nullptr) return;

	op->type = Operation::OperationType_SendStop;

	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		m_operationsSend.push_back(op);
	}
}

void SocketServer::disconnect(uint32 uSocketId)
{
	Operation* op = new Operation();
	if (op == nullptr) return;

	op->type = Operation::OperationType_SendCloseData;

	LOGWARNING("SocketServer Disconnect SocketId = %u.", uSocketId);

	op->socketID = uSocketId;
	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		m_operationsSend.push_back(op);
	}

}

void SocketServer::addOperation(uint32 type, uint32 socketID /* = 0 */, boost::array<char, Connection::MaxLengthBuffer> *buffer_recv_ /* = nullptr */, const std::string& ip /* = "" */)
{
	if (type == Operation::OperationType_Null) {
		return;
	}

	{
		boost::mutex::scoped_lock lock(m_mutexOperations);
		if (m_operations.size() >= Connection::MaxPendingSize * 1024)
		{
			////LOGINFO("%s:m_operations.size() >= Connection::MaxPendingSize * 1024:%d ",__FUNCTION__,Connection::MaxPendingSize * 1024);
			return;
		}
	}

	Operation* op = new Operation();
	if (op == nullptr) return;
	op->type = type;
	op->socketID = socketID;
	op->ip = ip;

	if (type == Operation::OperationType_RecvData && buffer_recv_ != nullptr)
	{
		uint32 recv_len = *((uint32*)buffer_recv_->data()) + sizeof(uint32);
		op->data = new char[recv_len];
		if (op->data == nullptr)
		{
			//LOGINFO("%s:op->data = %p ",__FUNCTION__,op->data);
			return;
		}
		memcpy(op->data, buffer_recv_->data(), recv_len);
	}
	{
		boost::mutex::scoped_lock lock(m_mutexOperations);
		m_operations.push_back(op);
	}
}

void SocketServer::handleAccept(const boost::system::error_code& e)
{
	if (!e)
	{
		boost::mutex::scoped_lock lock(m_mutexNewConnection);
		m_connectionMgr.startConnection(m_newConnection);
		m_newConnection.reset(new Connection(m_ioService, m_connectionMgr, *this));
		m_acceptor.async_accept(m_newConnection->socket(), boost::bind(&SocketServer::handleAccept, this, boost::asio::placeholders::error));
	}
}

void SocketServer::handleStop()
{
	m_acceptor.close();
	m_connectionMgr.stopAll();
	addOperation(Operation::OperationType_Stop);
}

bool SocketServer::setUserData(uint32 uSocketId, void* pUserData)
{
	boost::mutex::scoped_lock lock(m_mutexUserData);
	m_socketsUserData[uSocketId] = pUserData;
	if (pUserData == nullptr)
	{
		auto iter = m_socketsUserData.find(uSocketId);
		if (iter != m_socketsUserData.end())
			m_socketsUserData.erase(iter);
	}
	return true;
}

void* SocketServer::getUserData(uint32 uSocketId)
{
	boost::mutex::scoped_lock lock(m_mutexUserData);
	auto iter = m_socketsUserData.find(uSocketId);
	if (iter != m_socketsUserData.end())
		return iter->second;

	return nullptr;
}

std::string SocketServer::getRemoteIp(uint32 uSocketId)
{
	auto iter = m_socketPings.begin();
	if (iter == m_socketPings.end())
		return "";

	return iter->second.ip;
}

ISocketServer* createSocketServer()
{
	return new SocketServer();
}

void destroySocketServer(ISocketServer* pSocketServer)
{
	if (pSocketServer != nullptr)
		delete (SocketServer*)pSocketServer;
}
