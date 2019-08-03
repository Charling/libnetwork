//==========================================================================
/**
* @file	 : PacketDecoder.h
* @author : Charling(²éÁé)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : ÍøÂçÑ¹°ü
*/
//==========================================================================
#ifndef _PACKETENCODER_H_
#define _PACKETENCODER_H_

#include "Function.h"
#include "DataBlock.h"
#include "PacketDecoder.h"

class DataBlock;
class PacketDecoder;
class PacketEncoder
{
public:
	PacketEncoder();
	virtual~PacketEncoder();

public:
	virtual int32 getDataSize() const;
	virtual const void* getData() const;
	virtual DataBlock& getDataBlock() { return m_data; }

	virtual bool write(int8 val);
	virtual bool write(int16 val);
	virtual bool write(uint8 val);
	virtual bool write(uint16 val);
	virtual bool write(uint32 val);
	virtual bool write(uint64 val);
	virtual bool write(int32 val);
	virtual bool write(int64 val);
	virtual bool write(float val);
	virtual bool write(double val);
	virtual bool write(const char* val);
	virtual bool write(const std::string& val);
	virtual bool write(const DataBlock& val);
	virtual void clear();
	
	inline bool writeInt(int32 val)				{ return write(val);	}
	inline bool writeString(const std::string val) { return write(val); }

protected:
	DataBlock m_data;

};

template<typename T>
bool encodeData(PacketEncoder* encoder, const T& data)
{
	return data.encode(encoder);
}

template<typename T>
bool decodeData(const PacketDecoder* decoder, int32& index, T& data)
{
	return data.decode(decoder, index);
}

template<typename T>
bool encodeData(PacketEncoder* encoder, const std::list<T>& data)
{
	typedef typename std::list<T>::const_iterator iterator;

	iterator iter = data.begin();
	iterator iend = data.end();

	while(iter != iend)
	{
		if (encodeData(encoder, *iter) == false)
			return false;

		++iter;
	}

	return true;
}

template<typename T>
bool decodeData(const PacketDecoder* decoder, int32& index, std::list<T>& data)
{
	int32 length = 0;

	if (decoder->read(index - 1, length) == false)
		return false;

	for(int32 i = 0; i < length; i++)
	{
		data.push_back(T());

		if (decodeData(decoder, index, data.back()) == false)
			return false;
	}

	return true;
}

inline bool encodeData(PacketEncoder* encoder, int32 data)
{
	return encoder->write(data);
}

inline bool encodeData(PacketEncoder* encoder, const std::string& data)
{
	return encoder->write(data);
}

inline bool encodeData(PacketEncoder* encoder, const PacketDecoder& decoder,int32 nIndexStart = 0)
{
	int32 nCount = decoder.getCount();
	int32 nItemInt = 0;
	int64 nItemInt64 = 0;
	std::string strItemString = "";
	for (int32 i = nIndexStart; i < nCount; i++)
	{
		if (decoder.getType(i) == DataType_Int)
		{
			decoder.read(i,nItemInt);
			encoder->write(nItemInt);
		}
		if (decoder.getType(i) == DataType_String)
		{
			decoder.read(i, strItemString);
			encoder->write(strItemString);
		}
		if (decoder.getType(i) == DataType_Long)
		{
			decoder.read(i, nItemInt64);
			encoder->write(nItemInt64);
		}
	}
	return true;
}

inline bool EncodeData(PacketEncoder* encoder, PacketEncoder& encoderAdd)
{
	DataBlock& dbEncoder = encoder->getDataBlock();
	const DataBlock& dbEncoderAdd = encoderAdd.getDataBlock();

	int32 length = dbEncoderAdd.size() - 4;
	if (length <= 0)
		return false;

	dbEncoder.appendData((char *)dbEncoderAdd.data()+4, length);
	*reinterpret_cast<int32*>(dbEncoder.data()) += length;

	return true;
}

inline bool decodeData(const PacketDecoder* decoder, int32& index, int32& data)
{
	return decoder->read(index++, data);
}

inline bool decodeData(const PacketDecoder* decoder, int32& index, std::string& data)
{
	return decoder->read(index++, data);
}

#endif //_PACKETENCODER_H_