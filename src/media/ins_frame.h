#ifndef _INS_FRAME_H_
#define _INS_FRAME_H_

#include <stdint.h>
#include <memory>
#include "insbuff.h"
#include "metadata.h"
#include "pool_obj.h"

struct ins_frame : public pool_obj<ins_frame>
{
	virtual void clean()
	{
		page_buf = nullptr;
		buf = nullptr;
	}

	std::shared_ptr<page_buffer> page_buf; //video/image
	std::shared_ptr<insbuff> buf; //audio
	uint32_t pid = -1;
	uint8_t media_type;
	int64_t pts = 0;
	int64_t dts = 0;
	int64_t duration = 0;
	uint32_t sequence = 0;
	bool is_key_frame = false;
	bool b_fragment = false;
	jpeg_metadata metadata;
};

//decoded video or img data
struct ins_img_frame
{
	uint32_t w = 0;
	uint32_t h = 0;
	uint32_t fmt = 0;	
	std::shared_ptr<insbuff> buff;
};

struct ins_pcm_frame
{
	int64_t pts = -1;  
	std::shared_ptr<insbuff> buff;
};

struct ins_jpg_frame
{
	int64_t pts = -1;  
	std::shared_ptr<insbuff> buff;
};

#endif