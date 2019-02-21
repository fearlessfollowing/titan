
#ifndef _INS_BUFF_H_
#define _INS_BUFF_H_

#include "common.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "pool_obj.h"

class insbuff : public pool_obj<insbuff>
{
public:
	insbuff() = default;

	explicit insbuff(uint32_t size) 
	{
		alloc(size);
	}

	insbuff(uint8_t* data, uint32_t size, bool own = true) 
		: size_(size) 
		, own_(own)
		, data_(data)
	{
	}

	~insbuff()
	{
		if (own_ && data_) 
		{
			delete[] data_;
			data_ = nullptr;
		}
	}

	void alloc(uint32_t size)
	{
		size_ = size;
		data_ = new uint8_t[size_];
	}

	uint8_t* data()
	{
		return data_;
	}
	uint32_t size()
	{
		return size_;
	}
	void set_offset(uint32_t offset)
	{
		offset_ = offset;
	}
	uint32_t offset()
	{
		return offset_;
	}

private:
	uint8_t* data_ = nullptr;
	uint32_t size_ = 0;
	uint32_t offset_ = 0;
	bool own_ = true;
};

class page_buffer
{
public:
	page_buffer() = default;

	page_buffer(uint32_t size)
	{
		alloc(size);
	}
	
	void alloc(uint32_t size)
	{
		uint32_t page_size = getpagesize();
		size_ = ((size-1)/page_size + 1)*page_size;
		posix_memalign((void**)&data_, page_size, size_);
		memset(data_, 0x00, size_);
		offset_ = size;
	}

	~page_buffer()
	{
		INS_FREE(data_);
	}

	void set_offset(uint32_t offset)
	{
		offset_ = offset;
	}

	uint8_t* data()
	{
		return data_;
	}
	uint32_t size()
	{
		return size_;
	}
	uint32_t offset()
	{
		return offset_;
	}

private:
	uint8_t* data_ = nullptr;
	uint32_t size_ = 0;
	uint32_t offset_ = 0;
};

class insbuff2
{
public:
	insbuff2(unsigned size, bool b_page)
	{
		b_page_ = b_page;
		if (b_page_)
		{
			int page_size = getpagesize();
			size_ = ((size-1)/page_size + 1)*page_size;
			posix_memalign((void**)&data_, page_size, size_);
		}
		else
		{
			size_ = size;
			data_ = new unsigned char[size_];
		}
	}

	~insbuff2()
	{
		if (b_page_)
		{
			INS_FREE(data_);
		}
		else
		{
			INS_DELETE_ARRAY(data_);
		}
	}

	unsigned char* data()
	{
		return data_;
	}
	unsigned size()
	{
		return size_;
	}
	void set_offset(unsigned offset)
	{
		if (b_page_)
		{
			int page_size = getpagesize();
			offset_ = ((offset-1)/page_size + 1)*page_size;
		}
		else
		{
			offset_ = offset;
		}
	}
	unsigned offset()
	{
		return offset_;
	}

private:
	unsigned char* data_ = nullptr;
	unsigned size_ = 0;
	unsigned offset_ = 0;
	bool b_page_ = false;
};

// class insbuffex
// {
// public:
// 	insbuffex(const unsigned char* data, unsigned int size) : data_(data) , size_(size)
// 	{
// 	}

// 	~insbuffex()
// 	{
// 		if (data_ != nullptr)
// 		{
// 			free(const_cast<unsigned char*>(data_));
// 			data_ = nullptr;
// 			size_ = 0;
// 		}
// 	}
	
// 	const unsigned char* data()
// 	{
// 		return data_;
// 	}
// 	unsigned int size()
// 	{
// 		return size_;
// 	}
// 	void set_offset(unsigned int offset)
// 	{
// 		offset_ = offset;
// 	}
// 	unsigned int offset()
// 	{
// 		return offset_;
// 	}

// private:
// 	const unsigned char* data_ = nullptr;
// 	unsigned int size_ = 0;
// 	unsigned int offset_ = 0;
// };

#endif