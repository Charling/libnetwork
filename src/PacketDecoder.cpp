#include "Log.h"
#include "PacketDecoder.h"
#include "DataBlock.h"

PacketDecoder::PacketDecoder()
	: m_data(0)
{
	m_vecItems.clear();
}

PacketDecoder::~PacketDecoder()
{
	m_vecItems.clear();
}

bool PacketDecoder::setData(const char* buff, int length)
{
	if (buff == 0 || length < 4)
		return false;

	m_vecItems.clear();
	m_data = buff;

	int32 size = *reinterpret_cast<const int32*>(m_data);

	if (size + 4 != length)
		return false;

	const char* data = m_data;
	int32 i = 4;

	while (i < length && i > 0)
	{
		char type = data[i];
		int32 nNeedLen = 0;
		switch(type)
		{
		case DataType_Byte:
			{
				nNeedLen = 2;
				if (i+nNeedLen <= length)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_Word:
			{
				nNeedLen = 3;
				if (i+nNeedLen <= length)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_Int:
			{
				nNeedLen = 5;
				if (i+nNeedLen <= length)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_Long:
			{
				nNeedLen = 9;
				if (i + nNeedLen <= length)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_Float:
			{
				nNeedLen = 5;
				if (i+nNeedLen <= length)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_String:
			{
				nNeedLen = 5 + *reinterpret_cast<const int32*>(data + i + 1);
				if (i+nNeedLen <= length && i+nNeedLen>0)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_Double:
			{
				nNeedLen = 9;
				if (i+nNeedLen <= length)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
					return false;
				break;
			}
		case DataType_Binary:
			{
				nNeedLen = 5 + *reinterpret_cast<const int32*>(data + i + 1);
				if (i+nNeedLen <= length && i+nNeedLen>0)
				{
					m_vecItems.push_back(data + i);
					i += nNeedLen;
				}
				else
				{
					LOGERROR("nNeedLen:%d,i+nNeedLen:%d,length:%d,i+nNeedLen:%d",nNeedLen,i+nNeedLen,length,i+nNeedLen);
					return false;
				}
				break;
			}
		default:
			return false;
		}
	}

	return true;
}

int32 PacketDecoder::getCount() const
{
	return static_cast<int32>(m_vecItems.size());
}

emDataType PacketDecoder::getType(int idx) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return DataType_Unknown;

	const char* item = m_vecItems[idx];

	return static_cast<emDataType>(*item);
}

bool PacketDecoder::read(int idx, int8& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Byte)
		return false;

	item += 1;

	val = *reinterpret_cast<const int8*>(item);

	return true;
}

bool PacketDecoder::read(int idx, int16& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Word)
		return false;

	item += 1;

	val = *reinterpret_cast<const int16*>(item);

	return true;
}

bool PacketDecoder::read(int idx, uint8& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Byte)
		return false;

	item += 1;

	val = *reinterpret_cast<const uint8*>(item);

	return true;
}

bool PacketDecoder::read(int idx, uint16& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Word)
		return false;

	item += 1;

	val = *reinterpret_cast<const uint16*>(item);

	return true;
}

bool PacketDecoder::read(int idx, int32& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Int)
		return false;

	item += 1;

	val = *reinterpret_cast<const int32*>(item);

	return true;
}

bool PacketDecoder::read(int idx, int64& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Long)
		return false;

	item += 1;

	val = *reinterpret_cast<const int64*>(item);

	return true;
}

bool PacketDecoder::read(int idx, uint32& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Int)
		return false;

	item += 1;

	val = *reinterpret_cast<const uint32*>(item);

	return true;
}

bool PacketDecoder::read(int idx, uint64& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Long)
		return false;

	item += 1;

	val = *reinterpret_cast<const uint64*>(item);

	return true;
}

bool PacketDecoder::read(int idx, float& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Float)
		return false;

	item += 1;

	val = *reinterpret_cast<const float*>(item);

	return true;
}

bool PacketDecoder::read(int idx, double& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Double)
		return false;

	item += 1;

	val = *reinterpret_cast<const double*>(item);

	return true;
}

bool PacketDecoder::read(int idx, const char*& val) const
{
	val = 0;

	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_String)
		return false;

	item += 5;

	val = item;
	return true;
}

bool PacketDecoder::read(int idx, std::string& val) const
{
	const char* str = 0;

	if (this->read(idx, str) == false)
		return false;
	val = "";
	if (str != NULL)
	{
		int32 nSize = int32(*(int32*)(str - 4));
		if (nSize < 0 || nSize > 63*1024)
			return false;
		val = std::string(str,nSize);
	}
	return true;
}

bool PacketDecoder::read(int idx, DataBlock& val) const
{
	if (idx < 0 || idx >= static_cast<int32>(m_vecItems.size()))
		return false;

	const char* item = m_vecItems[idx];

	if (*item != DataType_Binary)
		return false;

	item += 1;

	int32 length = *reinterpret_cast<const int32*>(item);

	item += 4;

	val.appendData(item, length);

	return true;
}
