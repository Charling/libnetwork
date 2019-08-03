//==========================================================================
/**
* @file	 : PacketDecoder.h
* @author : Charling(²éÁé)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : ÍøÂç½â°ü
*/
//==========================================================================
#ifndef _PACKETDECODER_H_
#define _PACKETDECODER_H_

#include "Function.h"
#include "DataBlock.h"

enum emDataType
{
	DataType_Unknown = -1,					
	DataType_Byte,						
	DataType_Word,						
	DataType_Int,						
	DataType_Float,					
	DataType_String,					
	DataType_Object,					
	DataType_Double,					
	DataType_Binary,					
	DataType_Json	,					
	DataType_Long,						

	DataType_Max

};

class DataBlock;
class PacketDecoder
{
public:
	PacketDecoder();
	virtual~PacketDecoder();

public:
	virtual bool setData(const char* buff, int length);
	virtual const char* getData() { return m_data; };
	virtual int32 getCount() const;
	virtual emDataType getType(int idx) const;
	virtual bool read(int idx, int8& val) const;
	virtual bool read(int idx, int16& val) const;
	virtual bool read(int idx, uint8& val) const;
	virtual bool read(int idx, uint16& val) const;
	virtual bool read(int idx, int32& val) const;
	virtual bool read(int idx, int64& val) const;
	virtual bool read(int idx, uint32& val) const;
	virtual bool read(int idx, uint64& val) const;
	virtual bool read(int idx, float& val) const;
	virtual bool read(int idx, double& val) const;
	virtual bool read(int idx, const char*& val) const;
	virtual bool read(int idx, std::string& val) const;
	virtual bool read(int idx, DataBlock& val) const;

protected:
	std::vector<const char*> m_vecItems;
	const char* m_data;

};

#endif //_PACKETDECODER_H_