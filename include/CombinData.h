//==========================================================================
/**
* @file	 : CombinData.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2016-08-31  11:58
* Version : V2.01
* purpose : 组合包数据
*/
//==========================================================================
#ifndef _CombinData_H__
#define _CombinData_H__

#include "Function.h"
#include "INetwork.h"

class CombinData
{
public:
	CombinData();
	~CombinData();

public:
	size_t getCommbinLen();
	void clear(bool destructor = false);
	void fisrtPackage(const void* data, size_t nDataLen);
	void makePackage(const void* data, size_t nDataLen);
	void endPackage(uint32 socketId, ISocketHander* hander);

private:
	//合并发送的数据
	void* m_combinData;  
	//合并发送的数据长度
	size_t m_commbinLen;  
};

#endif