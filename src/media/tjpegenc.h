#ifndef _TJPEG_ENC_H_
#define _TJPEG_ENC_H_

#include "turbojpeg.h"
#include <memory>
#include <stdlib.h>
#include <stdint.h>
#include "ins_frame.h"

class tjpeg_enc
{
public:
	~tjpeg_enc();
	int32_t open(int32_t pix_fmt = TJPF_RGBA, int32_t flag = TJFLAG_BOTTOMUP);
	int32_t encode(const ins_img_frame& in_frame, ins_img_frame& out_frame, int32_t quality = 100);
private:
	tjhandle handle_ = nullptr;
	int32_t pix_fmt_ = TJPF_RGBA;
	int32_t flag_ = 0;
};

#endif