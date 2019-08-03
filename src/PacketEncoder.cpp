//==========================================================================
/**
* @file	 : PacketDecoder.cpp
* @author : Charling(²éÁé)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : ÍøÂçÑ¹°ü
*/
//==========================================================================
#include "PacketEncoder.h"
#include "DataBlock.h"

PacketEncoder::PacketEncoder()
{
	int32 length = 0;
	m_data.appendData(&length, 4);
}

PacketEncoder::~PacketEncoder()
{
}

int32 PacketEncoder::getDataSize() const
{
	return m_data.size();
}

const void* PacketEncoder::getData() const
{
	return m_data.data();
}

bool PacketEncoder::write(int8 val)
{
	char type = DataType_Byte;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 1);

	*reinterpret_cast<int8*>(m_data.data()) += 2;

	return true;
}

bool PacketEncoder::write(uint8 val)
{
	char type = DataType_Byte;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 1);

	*reinterpret_cast<uint8*>(m_data.data()) += 2;

	return true;
}

bool PacketEncoder::write(int16 val)
{
	char type = DataType_Word;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 2);

	*reinterpret_cast<int16*>(m_data.data()) += 3;

	return true;
}

bool PacketEncoder::write(uint16 val)
{
	char type = DataType_Word;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 2);

	*reinterpret_cast<uint16*>(m_data.data()) += 3;

	return true;
}

bool PacketEncoder::write(uint32 val)
{
	char type = DataType_Int;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 4);

	*reinterpret_cast<uint32*>(m_data.data()) += 5;

	return true;
}

bool PacketEncoder::write(uint64 val)
{
	char type = DataType_Long;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 8);

	*reinterpret_cast<uint64*>(m_data.data()) += 8;

	return true;
}

bool PacketEncoder::write(int32 val)
{
	char type = DataType_Int;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 4);

	*reinterpret_cast<int32*>(m_data.data()) += 5;

	return true;
}

bool PacketEncoder::write(int64 val)
{
	char type = DataType_Long;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 8);

	*reinterpret_cast<int32*>(m_data.data()) += 8;

	return true;
}

bool PacketEncoder::write(float val)
{
	char type = DataType_Float;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 4);

	*reinterpret_cast<int32*>(m_data.data()) += 5;

	return true;
}

bool PacketEncoder::write(double val)
{
	char type = DataType_Double;

	m_data.appendData(&type, 1);
	m_data.appendData(&val, 8);

	*reinterpret_cast<int32*>(m_data.data()) += 9;

	return true;
}

bool PacketEncoder::write(const char* val)
{

	char type = DataType_String;
	m_data.appendData(&type, 1);

	int32 length = 0;
	if(val != NULL)
	{
		length = (int32)(strlen(val));
		m_data.appendData(&length, 4);
		m_data.appendData(val, length);
	}
	else
	{
		static char tmp[16] = "\0";
		length = 1;
		m_data.appendData(&length, 4);
		m_data.appendData(&tmp, length);
	}

	*reinterpret_cast<int32*>(m_data.data()) += 5 + length;

	return true;
}
bool PacketEncoder::write(const std::string& val)
{
	char type = DataType_String;
	m_data.appendData(&type, 1);

	int32 length = 0;
	if(!val.empty())
	{
		length = (int32)(val.size());
		m_data.appendData(&length, 4);
		m_data.appendData(val.c_str(), (int32)val.size());
	}
	else
	{
		length = 1;
		m_data.appendData(&length, 4);
		static char tmp[16] = "\0";
		m_data.appendData(&tmp, 1);
	}

	*reinterpret_cast<int32*>(m_data.data()) += 5 + length;

	return true;
}

bool PacketEncoder::write(const DataBlock& val)
{
	char type = DataType_Binary;

	m_data.appendData(&type, 1);

	int32 length = val.size();

	m_data.appendData(&length, 4);
	m_data.appendData(val.data(), length);

	*reinterpret_cast<int32*>(m_data.data()) += 5 + length;

	return true;
}

void PacketEncoder::clear()
{
	m_data.deleteData(0, m_data.size());
	int32 length = 0;
	m_data.appendData(&length, 4);
}
