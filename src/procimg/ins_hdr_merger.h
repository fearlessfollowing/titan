#ifndef __INS_HDR_MERGER_H__
#define __INS_HDR_MERGER_H__

#include <vector>
#include <string>
#include <memory>
#include <stdint.h>
#include "ins_frame.h"

int32_t ins_hdr_merger(const std::vector<std::string>& in_file, ins_img_frame& out_img);

#endif