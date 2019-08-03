//==========================================================================
/**
* @file	 : DataBlock.h
* @author : Charling(查灵)/56430114@qq.com
* created : 2016-08-30  15:05
* Version : V2.01
* purpose : 二进制数据流，用于收发包数据流
*/
//==========================================================================
#ifndef _DATABLOCK_H__
#define _DATABLOCK_H__

#define MaxSignlePackSize 1024

class DataBlock
{
public:
	DataBlock();
	virtual~DataBlock();

public:
	virtual void* data();
	virtual const void* data() const;
	virtual const char* dataT() const;
	virtual int size() const;
	virtual int appendData(const void* buff, int size);
	virtual bool 	deleteData(int offset, int size);
	virtual void 	clearCache();
	virtual void 	init();
	virtual bool 	memcpyData(void* buff, int offset, int size);

public:
	template<typename T>
	T* dataT() {
		return reinterpret_cast<T*>(Data());
	};

	template<typename T>
	const T* dataT() const {
		return reinterpret_cast<const T*>(Data());
	};

protected:
	int	m_size;
	int	m_bufferSize;
	char* m_data;
};


#endif //_DATABLOCK_H__