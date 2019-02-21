
#include "tjpegenc.h"
#include "common.h"
#include "inslog.h"
#include <assert.h>

tjpeg_enc::~tjpeg_enc()
{
	if (handle_)
	{
		tjDestroy(handle_);
		handle_ = nullptr;
	}
}

int32_t tjpeg_enc::open(int32_t pix_fmt, int32_t flag)
{
	if (pix_fmt != TJPF_RGBA && pix_fmt != TJPF_BGR)
	{
		LOGERR("unsupport pixfmt:%d", pix_fmt);
		return INS_ERR_ENCODE;
	}

	if (flag != TJFLAG_BOTTOMUP && flag != 0)
	{
		LOGERR("unsupport flag:%d", flag);
		return INS_ERR;
	}

	pix_fmt_ = pix_fmt;
	flag_ = flag;

	handle_ = tjInitCompress();
	if (handle_)
	{
		//LOGINFO("tjpeg encoder open success");
		return INS_OK;
	}
	else
	{
		LOGERR("tjpeg encoder open fail:%s", tjGetErrorStr());
		return INS_ERR_ENCODE;
	}
}

int32_t tjpeg_enc::encode(const ins_img_frame& in_frame, ins_img_frame& out_frame, int32_t quality)
{
	assert(handle_ != nullptr);

	uint8_t* jpeg_buff = nullptr;
	uint64_t jpeg_size = 0;

	int32_t channels;
	if (pix_fmt_ == TJPF_RGBA)
	{
		channels = 4;
	}
	else 
	{
		channels = 3;
	}
	
	//jpeg_buff will be alloc by tjCompress2
	if (tjCompress2(handle_, in_frame.buff->data(), in_frame.w, in_frame.w*channels, in_frame.h, pix_fmt_, &jpeg_buff, &jpeg_size, TJSAMP_420, quality, flag_))
	{
		LOGERR("tjCompress2 fail:%s", tjGetErrorStr());
		return INS_ERR_ENCODE;
	}

	out_frame.w = in_frame.w;
	out_frame.h = in_frame.h;
	out_frame.buff = std::make_shared<insbuff>(jpeg_size);
	memcpy(out_frame.buff->data(), jpeg_buff, out_frame.buff->size());

	return INS_OK;
}