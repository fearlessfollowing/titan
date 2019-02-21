
#include "hdr_wrap.h"
#include "common.h"
#include "inslog.h"
#include "ins_hdr_merger.h"
#include <thread>

#define HDR_THREAD_CNT 1

void hdr_wrap::task(std::vector<std::string> file, int32_t index)
{
	int32_t ret = ins_hdr_merger(file, hdr_img_[index]);
	if (ret != INS_OK) result_ = ret;
}

int32_t hdr_wrap::process(const std::vector<std::vector<std::string>>& file, std::vector<ins_img_frame>& hdr_img)
{
	LOGINFO("hdr process begin");

	for (uint32_t i = 0; i < file.size(); i++)
	{
		ins_img_frame frame;
		hdr_img_.push_back(frame);
	}
	
	std::thread th[HDR_THREAD_CNT];

	for (int32_t j = 0; j < INS_CAM_NUM/HDR_THREAD_CNT; j++)
	{
		for (int32_t i = 0; i < HDR_THREAD_CNT; i++)
		{
			th[i] = std::thread(&hdr_wrap::task, this, file[i+j*HDR_THREAD_CNT], i+j*HDR_THREAD_CNT);
		}

		for (int32_t i = 0; i < HDR_THREAD_CNT; i++)
		{
			if (th[i].joinable()) th[i].join();
		}

		LOGINFO("hdr:%d process  result:%s", j, inserr_to_str(result_).c_str());
		
		RETURN_IF_NOT_OK(result_);

		if (progress_cb_) progress_cb_(1.0*HDR_THREAD_CNT/INS_CAM_NUM);
	}

	hdr_img = hdr_img_;

	LOGINFO("hdr process end success");

	return INS_OK;
}