//==========================================================================
/**
* @file	 : INetwork.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2014-02-24  15:08
* purpose : 各种需求的网络服务
*/
//==========================================================================
#ifndef __INetwork_h__
#define __INetwork_h__
#include "Log.h"
#include "Function.h"
#include "MemoryManager.h"
#include "PacketDecoder.h"
#include "PacketEncoder.h"
#include "PacketDispatcher.h"
//////////////////////////////////////////////////////////////////////////
//无效客户端socket id
#define INVALID_CLIENT_SOCKETID		0
//一个网络包的大小
#define MAX_NET_PACKAGE_SIZE 16384	//(16k)
//合并发送数据时特殊处理的填充字符
#define COMBIN_SEND_PADDING  0xBADDBADD
//消息头
#define HANDLER_KEY(type,id ) ((type << 16) + id)

#define GoogleProtobufParseFromString(msg,str) try{msg.ParseFromString(str);}catch(std::exception& e){LOGERROR("ParseFromString:%s,%d,%d",e.what(),msg.msg_type(),msg.msg_id());}
#define GoogleProtobufSerializeToString(msg,str) try{msg.SerializeToString(&str);}catch(std::exception& e){LOGERROR("SerializeToString:%s,%d,%d",e.what(),msg.msg_type(),msg.msg_id());}

//所有消息的消息头
struct MessageHeader
{
	int32 msgType;
	int32 msgId;

	bool operator < (const MessageHeader& b) const
	{
		return (msgType < b.msgType ||
			(msgType == b.msgType && msgId < b.msgId));
	}

	bool operator == (const MessageHeader& b) const
	{
		return (msgType == b.msgType && msgId == b.msgId);
	}

	bool operator != (const MessageHeader& b) const
	{
		return  (msgType != b.msgType || msgId != b.msgId);
	}
};

struct ISocketServerDelegate
{
	virtual void onRecvData(uint32 uSocketId, const MessageHeader& hdr, const PacketDecoder* pDecoder, void * pUserData) = 0;
	virtual void onConnect(uint32 uSocketId, const std::string& strIp) = 0;
	virtual void onDisconnect(uint32 uSocketId, const std::string& strIp) = 0;
	virtual void	onPing(uint32 uSocketId) {};
	virtual bool	canPing() { return false; };
};

typedef std::map< uint32, void* > VoidPointerMap;

struct ISocketServer
{
	virtual bool run(const char* address, const char*port, ISocketServerDelegate* pCallback) = 0;
	virtual void polling() = 0;
	virtual void sendData(uint32 uSocketId, PacketEncoder& encoder) = 0;
	virtual void disconnect(uint32 uSocketId) = 0;
	virtual void stop() = 0;
	virtual bool setUserData(uint32 uSocketId, void* pUserData) = 0;
	virtual void* getUserData(uint32 uSocketId) = 0;
	virtual VoidPointerMap& getUserData() = 0;
	virtual void setPingBreakInterval(uint32 interval) = 0;
	virtual std::string getRemoteIp(uint32 uSocketId) = 0;

	template<typename T>
	bool sendData(uint32 uSocketId, const T& msg)
	{
		static PacketEncoder encoder;

		encoder.clear();
		encoder.write(msg.msg_type());
		encoder.write(msg.msg_id());
		std::string result;
		GoogleProtobufSerializeToString(msg, result);
		encoder.write(result);
		sendData(uSocketId, encoder);

		return true;
	}
};

struct ISocketClientDelegate
{
	virtual void onConnect() = 0;
	virtual void onReconnect() = 0;
	virtual void onRecvData(const MessageHeader& hdr, const PacketDecoder* pDecoder) = 0;
	virtual void onDisconnect() = 0;
};

struct ISocketClient
{
	virtual bool connect(const char* address, const char* port, ISocketClientDelegate *pCallback, bool bAutoReconnect = false) = 0;
	virtual void polling() = 0;
	virtual void sendData(PacketEncoder& encoder) = 0;
	virtual void close() = 0;
	virtual void shutDown() = 0;
	virtual void setPingBreakInterval(uint32 interval) = 0;
	virtual bool isRunning() = 0;
	virtual bool reConnect() = 0;

	template<typename T>
	bool sendData(const T& msg)
	{
		static PacketEncoder encoder;

		encoder.clear();
		encoder.write(msg.msg_type());
		encoder.write(msg.msg_id());
		std::string result;
		try { msg.SerializeToString(&result); }
		catch (...) {}
		encoder.write(result);
		sendData(encoder);

		return true;
	}
};

struct ISocketHander
{
	virtual void onRecv(uint32 uSocketId, void *pData, size_t nDataLen) = 0;
};

extern "C"
{
	ISocketServer* createSocketServer();
	void destroySocketServer(ISocketServer* pSocketServer);
	ISocketClient* createSocketClient();
	void destroySocketClient(ISocketClient* pSocketClient);
}


#endif //__INetworkModule_h__