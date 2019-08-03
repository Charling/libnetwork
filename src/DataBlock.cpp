#include "DataBlock.h"
#include "Log.h"
#include "MemoryManager.h"

DataBlock::DataBlock()
	: m_size(0)
	, m_bufferSize(0)
	, m_data(nullptr)
{
}

DataBlock::~DataBlock()
{
	char* ptr = m_data;

	m_size = 0;
	m_bufferSize = 0;
	m_data = nullptr;
}

void* DataBlock::data()
{
	return m_data;
}

const void* DataBlock::data() const
{
	return m_data;
}

const char* DataBlock::dataT() const
{
	return m_data;
}

int DataBlock::size() const
{
	return m_size;
}

int DataBlock::appendData(const void* buff, int size)
{
	if (size <= 0)
		return 0;

	if (m_bufferSize - m_size < size)
	{
		auto newSize = (m_size + size + MaxSignlePackSize-1) / MaxSignlePackSize;
		newSize *= MaxSignlePackSize;

		char* data = new char[newSize];
		memcpy(data, m_data, m_size);

		char* temp = m_data;
		m_data = data;
		m_bufferSize = newSize;

		delete temp;
		temp = nullptr;
	}

	memcpy(m_data + m_size, buff, size);
	m_size += size;

	return size;
}

void DataBlock::clearCache()
{
	char* ptr = m_data;

	m_size = 0;
	m_bufferSize = 0;
	m_data = nullptr;

	delete ptr;
	ptr = nullptr;
	return;
}

void DataBlock::init()
{
	char* ptr = m_data;

	m_size = 0;
	m_bufferSize = 0;
	m_data = nullptr;

	delete ptr;
	ptr = nullptr;
}

bool DataBlock::deleteData(int offset, int size)
{
	if (offset >= m_size || size <= 0)
		return true;

	if (offset < 0)
	{
		size += offset;
		offset = 0;
	}

	if (offset + size > m_size)
		size = m_size - offset;

	if (m_size > MaxSignlePackSize && m_size / 2 < size)
	{
		auto newSize = (m_size - size + MaxSignlePackSize-1) / MaxSignlePackSize * MaxSignlePackSize;

		char* data = new char[newSize];
		if (offset > 0)
			memcpy(data, m_data, offset);

		if (offset + size < m_size)
			memcpy(data + offset, m_data + offset + size, m_size - offset - size);

		char* temp = m_data;

		m_bufferSize = newSize;
		m_size -= size;
		m_data = data;
		delete temp;
		temp = nullptr;

		return true;
	}
	else
	{
		if (offset + size >= m_size)
		{
			m_size = offset;
			return true;
		}
		else
		{
			memmove(m_data + offset, m_data + offset + size, m_size - offset - size);
			m_size -= size;
			return true;
		}
	}
}

bool DataBlock::memcpyData(void* buff, int offset, int size)
{
	if (offset < 0 || offset >= m_size || size <= 0 || m_data == nullptr || offset + size > m_size)
		return false;

	memcpy(buff, m_data + offset, size);
	return true;
}