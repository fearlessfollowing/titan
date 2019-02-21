
#ifndef _TJPEG_DEC_H_
#define _TJPEG_DEC_H_

#include "turbojpeg.h"
#include <memory>
#include <stdint.h>
#include "insbuff.h"
#include "ins_frame.h"

class tjpeg_dec
{
public:
	~tjpeg_dec();
	int32_t open(int pix_fmt = TJPF_RGBA);
	int32_t decode(const uint8_t* data, uint32_t size, ins_img_frame& frame);
private:
	tjhandle handle_ = nullptr;
	int pix_fmt_ = TJPF_RGBA;
};

#endif