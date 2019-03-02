
#include "tjpegdec.h"
#include "common.h"
#include "inslog.h"

tjpeg_dec::~tjpeg_dec()
{
	if (handle_) {
		tjDestroy(handle_);
		handle_ = nullptr;
	}
}

int32_t tjpeg_dec::open(int32_t pix_fmt)
{
	if (pix_fmt != TJPF_RGBA && pix_fmt != TJPF_BGR) {
		LOGERR("unsupport pixfmt:%d", pix_fmt);
		return INS_ERR_DECODE;
	}

	pix_fmt_ = pix_fmt;

	handle_ = tjInitDecompress();
	if (handle_) {
		//LOGINFO("tjpeg decode open success");
		return INS_OK;
	} else {
		LOGERR("tjpeg decode open fail:%s", tjGetErrorStr());
		return INS_ERR_DECODE;
	}
}

int32_t tjpeg_dec::decode(const uint8_t* data, uint32_t size, ins_img_frame& frame)
{
	int channels;

	if (pix_fmt_ == TJPF_RGBA) {
		channels = 4;
	} else  {
		channels = 3;
	}

	if (handle_ == nullptr) {
		LOGERR("handle is nullptr");
		return INS_ERR;
	}

	int width, height, sub_samp, color_sapce;

	if (tjDecompressHeader3(handle_, data, size, &width, &height, &sub_samp, &color_sapce)) {
		LOGERR("tjDecompressHeader fail:%s", tjGetErrorStr());
		return INS_ERR;
	}

	frame.buff = std::make_shared<insbuff>(width*height*channels);
	frame.w = width;
	frame.h = height;

	if (tjDecompress2(handle_, data, size, frame.buff->data(), width, width *channels , height, pix_fmt_, TJFLAG_FASTDCT)) {
		LOGERR("tjDecompress2 fail:%s", tjGetErrorStr());
		return INS_ERR;
	}

	return INS_OK;
}
