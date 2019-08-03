//==========================================================================
/**
* @file	 : CombinData.h
* @author : Charling(����)/56430114@qq.com
* created : 2016-08-31  11:58
* Version : V2.01
* purpose : ��ϰ�����
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
	//�ϲ����͵�����
	void* m_combinData;  
	//�ϲ����͵����ݳ���
	size_t m_commbinLen;  
};

#endif