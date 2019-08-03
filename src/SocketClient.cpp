#include "SocketClient.h"
#include "log.h"
using namespace Base;

SocketClient::SocketClient()
	: m_socket(m_ioService)
	, m_socketClientCallback(nullptr)
	, m_bufferDataSize(0)
	, m_writing(false)
	, m_running(false)
	, m_preRecvTime(0)
	, m_preCheckTime(0)
	, m_autoReconnect(false)
	, m_writingData(false)
	, m_thread(nullptr)
	, m_pingBreakInterval(0)
{
	m_combinData = new CombinData();
}

SocketClient::~SocketClient()
{
	clear();
	do
	{
		std::list< char* >::iterator iter = m_writes.begin();
		if (iter == m_writes.end())
			break;
		char* p = *iter;
		m_writes.pop_front();
		delete p;
	} while (1);

	stop();

	if (m_thread != nullptr)
	{
		m_thread->join();
		delete m_thread;
		m_thread = nullptr;

		m_writingData = false;
	}

	delete m_combinData;
	m_combinData = nullptr;
}

void SocketClient::clear()
{
	do
	{
		OperationClient * op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperations);
			if (m_operations.size() > 0)
			{
				op = *(m_operations.begin());
				m_operations.pop_front();
			}
		}

		if (op == nullptr)
			break;
		delete op;
		op = nullptr;
	} while (1);

	{
		boost::mutex::scoped_lock lock(m_mutexOperations);
		m_operations.clear();
	}

	do
	{
		OperationClient * op = nullptr;
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

	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		m_operationsSend.clear();
	}
}

bool  SocketClient::reConnect()
{
	if (m_autoReconnect)
	{
		//	LOGINFO("%s,ip:%s,port:%s ",__FUNCTION__,m_ip.c_str(), m_port.c_str());
		try
		{
			boost::asio::ip::tcp::resolver resolver(m_ioService);
			boost::asio::ip::tcp::resolver::query query(m_ip.c_str(), m_port.c_str());
			boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
			//boost::asio::ip::tcp::endpoint endpoint = *iterator;
			m_socket.connect(*iterator);
		}
		catch (std::exception& e)
		{
			LOGINFO("ReConnect Exception:%s,ip:%s,port:%s", e.what(), m_ip.c_str(), m_port.c_str());
			boost::system::error_code ignored_ec;
			//m_socket.cancel(ignored_ec);
			boost::asio::socket_base::linger linger_op(true, 0);
			m_socket.set_option(linger_op);
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			m_socket.close(ignored_ec);
			return false;
		}

		addOperation(OperationClient::OperationClientType_ReConnect);
		start();
		m_running = true;
		{
			boost::mutex::scoped_lock lock(m_mutexWritingData);
			m_writingData = false;
		}

		if (m_thread == nullptr)
			m_thread = new boost::thread(boost::bind(&SocketClient::runLoop, this));

		return true;
	}

	return false;
}

bool SocketClient::connect(const char* address, const char* port, ISocketClientDelegate* pCallback, bool bAutoReconnect)
{
	if (pCallback == nullptr) {
		return false;
	}
	try
	{
		boost::asio::ip::tcp::resolver resolver(m_ioService);
		boost::asio::ip::tcp::resolver::query query(address, port);
		boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
		m_socketClientCallback = pCallback;
		m_socket.connect(*iterator);
		m_autoReconnect = bAutoReconnect;
		m_ip = address;
		m_port = port;
	}
	catch (std::exception& e)
	{
		try
		{
			LOGERROR("Connect Exception:%s,ip:%s,port:%s", e.what(), address, port);
			boost::system::error_code ignored_ec;
			boost::asio::socket_base::linger linger_op(true, 0);
			m_socket.set_option(linger_op);
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			m_socket.close(ignored_ec);
		}
		catch (std::exception& e)
		{
			LOGERROR("Exception:%s ", e.what());
		}
		return false;
	}
	catch (...)
	{
		try
		{
			boost::system::error_code ignored_ec;
			boost::asio::socket_base::linger linger_op(true, 0);
			m_socket.set_option(linger_op);
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
			m_socket.close(ignored_ec);
		}
		catch (std::exception& e)
		{
			LOGINFO("Exception:%s ", e.what());
		}
		return false;
	}
	addOperation(OperationClient::OperationClientType_Connect);
	start();
	m_running = true;
	{
		boost::mutex::scoped_lock lock(m_mutexWritingData);
		m_writingData = false;
	}
	if (m_thread == nullptr)
		m_thread = new boost::thread(boost::bind(&SocketClient::runLoop, this));
	return true;


}
void SocketClient::runLoop()
{
	do
	{
		try
		{
			if (m_ioService.stopped())
				m_ioService.reset();
			m_ioService.run();
		}
		catch (...)
		{
			LOGINFO("%s catch!!!!!!!! ", __FUNCTION__);
		}
	} while (1);

	LOGWARNING("%s RunLoop end.........", __FUNCTION__);
}

void SocketClient::checkOperation()
{
	if (getTickCount() - m_preCheckTime > m_pingBreakInterval / 10)
	{
		if (m_preRecvTime != 0 && getTickCount() - m_preRecvTime > m_pingBreakInterval)
		{
			close();
			m_preRecvTime = 0;
			//	LOGINFO("%s getTickCount() - m_preRecvTime > m_pingBreakInterval:%d", __FUNCTION__, m_pingBreakInterval);
		}
		m_preCheckTime = getTickCount();
	}
}

void SocketClient::polling()
{
	if (m_socketClientCallback == nullptr)
		return;

	//	m_socketClientCallback->OnUpdate();

	{
		boost::mutex::scoped_lock lock(m_mutexWritingData);
		if (!m_writingData)
		{
			m_ioService.post(boost::bind(&SocketClient::writeData, this));
			m_writingData = true;

		}
	}

	uint32 count = 0;
	uint32 operations_size = 0;
	do
	{
		checkOperation();

		OperationClient * op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperations);
			if (m_operations.size() > 0)
			{
				op = *(m_operations.begin());
				m_operations.pop_front();
				if (operations_size == 0)
					operations_size = (uint32)(m_operations.size());
			}
		}

		if (op == nullptr) break;

		switch (op->type)
		{
		case OperationClient::OperationClientType_Connect:
		{
			m_socketClientCallback->onConnect();
			m_preRecvTime = getTickCount();
			//	LOGINFO("%s OperationClient::OperationClientType_Connect ",__FUNCTION__);
		} break;
		case OperationClient::OperationClientType_ReConnect:
		{
			m_socketClientCallback->onReconnect();
			//	LOGINFO("%s OperationClient::OperationClientType_ReConnect ",__FUNCTION__);
			m_preRecvTime = getTickCount();
		} break;
		case OperationClient::OperationType_CloseConnection:
		{
			m_socketClientCallback->onDisconnect();
			m_preRecvTime = 0;
			//	LOGINFO("%s OperationClient::OperationType_CloseConnection m_preRecvTime = 0;",__FUNCTION__);
		} break;
		case OperationClient::OperationType_RecvData: 
		{
			recvData(op);
			delete op->data;
			op->data = nullptr;
			m_preRecvTime = getTickCount();
		} break;
		default:
			break;
		}
		delete op;
		op = nullptr;
		count++;
		if (count >= operations_size)
			break;
	} while (1);

}

void SocketClient::onRecv(uint32 uSocketId, void* data, size_t len)
{
	if (len < sizeof(MessageHeader))
	{
		LOGERROR("DataLen < sizeof(MessageHeader)");
		return;
	}

	PacketDecoder decoder;
	if (decoder.setData(reinterpret_cast<const char*>(data), (int32)(len + 4)) == false)
	{
		LOGERROR("error :bad request1");
		return;
	}

	if (decoder.getCount() < 2)
	{
		LOGERROR("error :bad request decoder.getCount() < 2");
		return;
	}

	MessageHeader hdr;
	if (decoder.read(0, hdr.msgType) == false || decoder.read(1, hdr.msgId) == false)
	{
		LOGERROR("error :bad request");
		return;
	}

	if (hdr.msgType == 0 && hdr.msgId == 0)
	{//server socket推送ping的消息，必须反馈，不然就会被踢出了
		int32 interval = 0;
		if (decoder.read(2, interval))
			m_pingBreakInterval = interval;

		static PacketEncoder encoder;
		encoder.clear();
		encoder.write(int32(0));
		encoder.write(int32(0));
		sendData(encoder);

		return;
	}

	if (hdr.msgId < 0 || hdr.msgId >= 65536)
	{
		LOGERROR("error :method id:%d,is out of range", hdr.msgId);
		return;
	}

	PacketDecoder* pDecoder = &decoder;
	m_socketClientCallback->onRecvData(hdr, pDecoder);
}

void SocketClient::recvData(OperationClient* op)
{
	IF_NOT_RETURN(op);
	IF_NOT_RETURN(m_combinData);

	//发送的数据长度超过了默认的组包算法最大长度
	//合并发送的思路:
	//1.当发送的数据包大于预先设定的值时,系统自动将这个包拆开分成多个包进行发送
	//2.当接受方发现收到一个MAX_NET_PACKAGE_SIZE大小的数据包时，就表示需要和后面那个数据包合并
	//3.需要特别处理的是，如果发送的数据包正好是MAX_NET_PACKAGE_SIZE字节,则在后面追加4个特殊字节(0xBADDBADD),这种情况出现的概率非常低

	void* data = op->data;
	size_t len = *((uint32*)op->data);

	if (m_combinData->getCommbinLen() > 0)
	{//目前处于组合包
		char* pRealData = reinterpret_cast<char*>(data)+4;

		if (len == 4 && *((uint32*)(pRealData)) == COMBIN_SEND_PADDING)
		{//终结包,此概率极低
			m_combinData->endPackage(0, this);
			return;
		}

		//组建包
		m_combinData->makePackage(pRealData, len);

		if (len != MAX_NET_PACKAGE_SIZE - 1)
		{//终结包
			m_combinData->endPackage(0, this);
		}

		return;
	}

	if (len == MAX_NET_PACKAGE_SIZE - 1)
	{//收到MAX_NET_PACKAGE_SIZE的包就表示后续还有包
		//首包,包括长度4个字节
		m_combinData->fisrtPackage(data, len + 4);

		return;
	}

	onRecv(0, data, len);
}

void SocketClient::start()
{
	m_socket.async_read_some(boost::asio::buffer(m_arrBuffer),
		boost::bind(&SocketClient::handleRead, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void SocketClient::stop()
{
	LOGINFO("%s ip:%s,port:%s", __FUNCTION__, m_ip.c_str(), m_port.c_str());

	if (m_socket.is_open())
	{
		boost::system::error_code ignored_ec;
		//m_socket.cancel(ignored_ec);
		boost::asio::socket_base::linger linger_op(true, 0);
		m_socket.set_option(linger_op);
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		m_socket.close(ignored_ec);
		addOperation(OperationClient::OperationType_CloseConnection, &m_arrBuffer);

		{
			boost::mutex::scoped_lock lock(m_mutexRunning);
			m_running = false;
		}

		LOGINFO("%s ip:%s,port:%s...............", __FUNCTION__, m_ip.c_str(), m_port.c_str());
		clear();
	}
}

void SocketClient::addOperation(uint32 type, boost::array<char, MaxLengthBuffer> *buffer_recv_)
{
	if (type == OperationClient::OperationType_Null) {
		return;
	}

	{
		boost::mutex::scoped_lock lock(m_mutexOperations);
		if (m_operations.size() >= MaxPendingSize * 1024)
		{
			return;
		}
	}

	OperationClient* op = new OperationClient;
	if (op == nullptr) return;
	op->type = type;

	if (type == OperationClient::OperationType_RecvData && buffer_recv_ != nullptr)
	{
		//uint32 recv_len = *((uint32*)buffer_recv_->data());
		uint32 recv_len = *((uint32*)buffer_recv_->data()) + sizeof(uint32);
		op->data = (char*)allocateMemory(recv_len);
		if (op->data == nullptr)
		{
			////LOGINFO("%s:op->data = %p ",__FUNCTION__,op->data);
			return;
		}
		memcpy(op->data, buffer_recv_->data(), recv_len);
	}

	{
		boost::mutex::scoped_lock lock(m_mutexOperations);
		m_operations.push_back(op);
	}

}

void SocketClient::popOneByte()
{
	if (m_bufferDataSize > 0)
	{
		m_bufferDataSize -= 1;
		if (m_bufferDataSize > 0) {
			memmove(m_arrBuffer.data(), m_arrBuffer.data() + 1, m_bufferDataSize);
		}
	}
}

void SocketClient::handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
{

	if (!e)
	{
		m_bufferDataSize += bytes_transferred;
		//LOGINFO("SocketClient::HandleRead:%d ",bytes_transferred);
		uint32 miss_byte_count = 0;
		while (1)
		{
			if (m_bufferDataSize <= sizeof(uint32))
				break;

			//uint32 nNeedRecvLen = *((uint32*)m_arrBuffer.data())
			uint32 nNeedRecvLen = *((uint32*)m_arrBuffer.data()) + sizeof(uint32);//for love island;

			if (nNeedRecvLen < sizeof(uint32) || nNeedRecvLen > MaxLengthBuffer)
			{
				//LOGERROR("%s: next_read_len < 0 || (next_read_len + m_bufferDataSize) > MaxLengthBuffer,next_read_len = %d ",__FUNCTION__,next_read_len);("%s: nNeedRecvLen < sizeof(uint32) || nNeedRecvLen > MaxLengthBuffer :nNeedRecvLen = %d ",__FUNCTION__,nNeedRecvLen);
				//m_bufferDataSize = 0;
				//break;
				popOneByte();
				miss_byte_count++;
				continue;
			}

			if (nNeedRecvLen == sizeof(uint32))
			{
				//LOGINFO("%s: nNeedRecvLen = sizeof(uint32) :nNeedRecvLen = %d ",__FUNCTION__,nNeedRecvLen);
				//m_bufferDataSize = 0;
				//break;
				popOneByte();
				miss_byte_count++;
				continue;
			}

			if (m_bufferDataSize < nNeedRecvLen) //not enough
			{
				break;
			}

			addOperation(OperationClient::OperationType_RecvData, &m_arrBuffer);
			//m_bufferDataSize = 0;
			m_bufferDataSize -= nNeedRecvLen;
			if (m_bufferDataSize > 0)
			{
				memmove(m_arrBuffer.data(), m_arrBuffer.data() + nNeedRecvLen, m_bufferDataSize);
			}


		}

		int32 next_read_len = 0;
		if (m_bufferDataSize < sizeof(uint32))
		{
			next_read_len = sizeof(uint32) - (uint32)m_bufferDataSize;
		}
		else
		{

			do
			{
				next_read_len = *((uint32*)m_arrBuffer.data()) + sizeof(uint32) - (uint32)m_bufferDataSize;//for love island;
				if (next_read_len > 0 && (next_read_len + m_bufferDataSize) <= MaxLengthBuffer)
				{
					//LOGERROR("%s: next_read_len < 0 || (next_read_len + m_bufferDataSize) > MaxLengthBuffer,next_read_len = %d ",__FUNCTION__,next_read_len);
					break;
				}
				popOneByte();
				miss_byte_count++;
				if (m_bufferDataSize < sizeof(uint32))
				{
					next_read_len = sizeof(uint32) - (uint32)m_bufferDataSize;
					break;
				}

			} while (1);
		}
		m_socket.async_read_some(boost::asio::buffer(m_arrBuffer.data() + m_bufferDataSize, next_read_len),//MaxLengthBuffer - m_bufferDataSize),
			boost::bind(&SocketClient::handleRead, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	}
	else if (e != boost::asio::error::operation_aborted) {
		stop();
	}

}

void SocketClient::handleWrite(const boost::system::error_code& e)
{
	m_writing = false;
	if (!m_socket.is_open())
	{
		stop();
		//LOGINFO("%s stopped_ ",__FUNCTION__);
		return;
	}
	if (!e)
	{
		char* p = nullptr;
		if (m_writes.size() > 0)
		{
			std::list< char* >::iterator iter = m_writes.begin();
			p = *iter;
			//write(p,*((uint32*)p));
			m_writes.pop_front();
		}

		if (p != nullptr)
		{
			write(p, *((uint32*)p));
		}
	}

	else if (e != boost::asio::error::operation_aborted)
	{
		stop();
		//LOGINFO("%s e != boost::asio::error::operation_aborted ",__FUNCTION__);
	}


}

void SocketClient::sendData(PacketEncoder& encoder)
{
	//发送的数据长度超过了默认的组包算法最大长度
	//合并发送的思路:
	//1.当发送的数据包大于预先设定的值时,系统自动将这个包拆开分成多个包进行发送
	//2.当接受方发现收到一个MAX_NET_PACKAGE_SIZE大小的数据包时，就表示需要和后面那个数据包合并
	//3.需要特别处理的是，如果发送的数据包正好是MAX_NET_PACKAGE_SIZE字节,则在后面追加4个特殊字节(0xBADDBADD),这种情况出现的概率非常低

	int32 size = encoder.getDataSize();

	if (size <= MAX_NET_PACKAGE_SIZE)
	{//考虑到客户端每个包不可能超过16k,所以正常包不能受影响
		if (size == 0)
			return;

		void *p = const_cast<void*>(encoder.getData());

		sendData((char*)p + sizeof(uint32), size - sizeof(uint32));
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
			sendData(value, needSize);

			pRealData += needSize;
			size -= needSize;
			totalSize += needSize;
			delete value;

			if (size == 0 && needSize == MAX_NET_PACKAGE_SIZE - 1)
			{//没办法，只能特殊处理了,通知此包已经结尾
				uint32* nPadding = new uint32[sizeof(uint32)];
				*((uint32*)(nPadding)) = COMBIN_SEND_PADDING;
				sendData((char*)nPadding, sizeof(uint32));
				delete nPadding;
			}
		}
	}
}

void SocketClient::sendData(const void* data, size_t len)
{
	if (data == nullptr || len == 0)
	{
		////LOGINFO("Error:%s data == nullptr || len == 0 len = %d ",__FUNCTION__,len);
		return;
	}

	if (len + sizeof(uint32) > MaxLengthBuffer)
	{
		LOGERROR("%s len + sizeof(uint32) > MaxLengthBuffer:%d ", __FUNCTION__, len);
		return;
	}

	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		if (m_operationsSend.size() >= MaxPendingSize * 1024)
		{
			////LOGINFO("%s:m_operationsSend.size() >= MaxPendingSize * 1024:%d ",__FUNCTION__,MaxPendingSize * 1024);
			return;
		}
	}

	OperationClient* op = new OperationClient();
	if (op == nullptr) {
		return;
	}
	op->type = OperationClient::OperationType_Null;
	op->data = (char*)allocateMemory(len + sizeof(uint32));
	if (op->data == nullptr)
	{
		////LOGINFO("%s:op->data = %p ",__FUNCTION__,op->data);
		return;
	}
	*((uint32*)op->data) = (uint32)(len + sizeof(uint32));
	//*((uint32*)op->data) = (uint32)(len);//for love island;
	memcpy(op->data + sizeof(uint32), data, len);

	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		m_operationsSend.push_back(op);

		boost::mutex::scoped_lock locker(m_mutexWritingData);
		if (!m_writingData)
		{
			m_ioService.post(boost::bind(&SocketClient::writeData, this));
			m_writingData = true;
		}
	}

}

void SocketClient::write(char* data, size_t len)
{
	if (!m_socket.is_open())
	{
		delete data;
		return;
	}

	if (len > MaxLengthBuffer)
	{
		LOGERROR("%s len > MaxLengthBuffer:len:%d ", __FUNCTION__, len);
		delete data;
		return;
	}

	if (m_writing)
	{
		if (m_writes.size() >= MaxPendingSize)
		{
			////LOGINFO("%s m_writes.size() >= MaxPendingSize:%d ",__FUNCTION__,MaxPendingSize);
			delete data;
			return;
		}
		m_writes.push_back(data);
		return;
	}

	size_t nSendLen = 0;

	//*((uint32*)mArrwriteBuffer.data()) = (uint32)len;
	memcpy(m_arrWriteBuffer.data(), data, len);
	*((uint32*)m_arrWriteBuffer.data()) = (uint32)len - sizeof(uint32);//for love island;
	nSendLen += len;

	boost::asio::async_write(m_socket,
		boost::asio::buffer(m_arrWriteBuffer.data(), nSendLen),
		boost::bind(&SocketClient::handleWrite, this, boost::asio::placeholders::error));

	m_writing = true;
	delete data;
	return;
}

void SocketClient::writeData()
{
	if (!m_socket.is_open())
	{
		reConnect();
		{
			boost::mutex::scoped_lock lock(m_mutexWritingData);
			m_writingData = false;
		}
		return;
	}

	uint32 count = 0;//_GetTickCount();
	uint32 operations_size = 0;
	do
	{
		OperationClient * op = nullptr;
		{
			boost::mutex::scoped_lock lock(m_mutexOperationsSend);
			if (m_operationsSend.size() > 0)
			{
				op = *(m_operationsSend.begin());
				m_operationsSend.pop_front();
				if (operations_size == 0)
					operations_size = (uint32)(m_operationsSend.size());
			}
		}
		if (op == nullptr)
			break;
		switch (op->type)
		{
			case OperationClient::OperationType_SendData:
			{
				write(op->data, *((uint32*)op->data));
				op->data = nullptr;
			} break;
			case OperationClient::OperationType_SendCloseData:
			{
				stop();
			} break;
			case OperationClient::OperationClientType_SendShutDownConnection:
			{
				stop();
			} break;
			default:
				break;
		}
		delete op;
		op = nullptr;

		count++;
		if (count >= operations_size)
			break;
	} while (1);


	{
		boost::mutex::scoped_lock lock(m_mutexWritingData);
		m_writingData = false;
	}
}

void SocketClient::close()
{
	if (!isRunning())
		return;
	if (!m_socket.is_open())
		return;

	OperationClient* op = safeCreateObject(OperationClient);
	if (op == nullptr) return;
	op->type = OperationClient::OperationType_SendCloseData;
	{
		boost::mutex::scoped_lock lock(m_mutexOperationsSend);
		m_operationsSend.push_back(op);
	}

	{
		boost::mutex::scoped_lock lock(m_mutexRunning);
		m_running = false;
	}

	LOGINFO("%s ", __FUNCTION__);
}

void SocketClient::shutDown()
{
	if (!isRunning())
		return;
	{
		boost::mutex::scoped_lock lock(m_mutexRunning);
		m_running = false;
	}

	if (m_socket.is_open())
	{
		//boost::mutex::scoped_lock lock(mutex_socket_);
		boost::system::error_code ignored_ec;
		//m_socket.cancel(ignored_ec);
		boost::asio::socket_base::linger linger_op(true, 0);
		m_socket.set_option(linger_op);
		m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		m_socket.close(ignored_ec);
		m_socketClientCallback->onDisconnect();
	}

}

ISocketClient* createSocketClient()
{
	return new SocketClient();
}

void destroySocketClient(ISocketClient* pSocketClient)
{
	if (pSocketClient != nullptr)
		delete (SocketClient*)pSocketClient;
}
