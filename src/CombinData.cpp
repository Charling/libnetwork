#include "CombinData.h"
#include "MemoryManager.h"
#include "log.h"
using namespace Base;

CombinData::CombinData()
	: m_combinData(nullptr)
	, m_commbinLen(0)
{
}

CombinData::~CombinData()
{
	clear(true);
}

void CombinData::clear(bool destructor /* = false */)
{
	if (destructor == true && m_commbinLen > 0)
	{//�����Ļ����˴���Ȼm_commbinLen<=0��������������bug
		LOGERROR("destructor == true && m_commbinLen > 0....");
	}

	if (m_commbinLen > 0)
	{
		delete m_combinData;
		m_combinData = nullptr;
		m_commbinLen = 0;
	}
}

size_t CombinData::getCommbinLen()
{
	return m_commbinLen;
}

void CombinData::fisrtPackage(const void* data, size_t nDataLen)
{
	//�����˼�¼���ȵ�4���ֽ�
	m_combinData = (void*) ::operator new (nDataLen);
	m_commbinLen = nDataLen;
	memcpy(m_combinData, data, m_commbinLen);
}

void CombinData::makePackage(const void* data, size_t nDataLen)
{
	//nDataLenΪ��ʵ����
	void* buff = (void*) ::operator new (nDataLen + m_commbinLen);
	memcpy(buff, m_combinData, m_commbinLen);
	memcpy((char *)buff + m_commbinLen, data, nDataLen);
	delete m_combinData;
	m_combinData = buff;
	m_commbinLen += nDataLen;
}

void CombinData::endPackage(uint32 socketId, ISocketHander* hander)
{
	//��ȥ�׸������ֽ�
	m_commbinLen -= 4;

	*((uint32*)m_combinData) = (uint32)(m_commbinLen);
	hander->onRecv(socketId, m_combinData, m_commbinLen);

	delete m_combinData;
	m_combinData = nullptr;
	m_commbinLen = 0;
}