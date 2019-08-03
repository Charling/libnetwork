//==========================================================================
/**
* @file	 : PacketDispatcher.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : 消息转发器
*/
//==========================================================================
#ifndef __PACKET_DISPATCHER_H__
#define __PACKET_DISPATCHER_H__

#include "Function.h"
#include "Log.h"

template <class T> 
class PacketDispatcher
{
public:
	PacketDispatcher()
	{
		m_defaultHandler = NULL;
		m_mapHandlers.clear();
	}
	virtual~PacketDispatcher()
	{
		m_defaultHandler = NULL;
		m_mapHandlers.clear();
	}

	void RegisterHandler(int32 id, T handler)
	{
		typename std::map<int32, T >::iterator iter = m_mapHandlers.find(id);
		if(iter != m_mapHandlers.end())
		{
			LOGINFO("iter != m_mapHandlers.end(),id:%d ",id);
			return;
		}
		m_mapHandlers[id] = handler;

	}

	T GetHandler(int32 id) {
		typename std::map<int32, T >::iterator iter = m_mapHandlers.find(id);
		if(iter == m_mapHandlers.end())
		{
			return m_defaultHandler;
		}
		return iter->second;
	}

	bool hasHandler(int32 id) {
		return m_mapHandlers.find(id) != m_mapHandlers.end();
	}

	void setDefaultHandler(T handler){ m_defaultHandler = handler;};


private:
	std::map<int32, T > m_mapHandlers;
	T m_defaultHandler;

};

#endif //__PACKET_DISPATCHER_H__