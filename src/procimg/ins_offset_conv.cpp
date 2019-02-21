

#include "ins_offset_conv.h"
#include "common.h"
#include "inslog.h"
#include "OffsetConvert/OffsetConvert.h"

//in 为4:3pic的offset
int32_t ins_offset_conv::conv(const std::string& in, std::string& out, int32_t crop_flag)
{
    ins::OffsetConvert::CONVERTTYPE type;
    switch (crop_flag)
	{
        case INS_CROP_FLAG_PIC:
            out = in;
            return INS_OK;  //不用转换
		case INS_CROP_FLAG_4_3:
            type = ins::OffsetConvert::CONVERTTYPE::PRO2_4000_3000_3840_2880;
			break;
		case INS_CROP_FLAG_16_9:
            type = ins::OffsetConvert::CONVERTTYPE::PRO2_4000_3000_3840_2160;
			break;
		case INS_CROP_FLAG_2_1:
            type = ins::OffsetConvert::CONVERTTYPE::PRO2_4000_3000_3840_1920;
			break;
        case INS_CROP_FLAG_11:
            type = ins::OffsetConvert::CONVERTTYPE::TITAN_5280_3956_5280_2972;
            break;
        case INS_CROP_FLAG_12:
            type = ins::OffsetConvert::CONVERTTYPE::TITAN_5280_3956_5038_3380;
            break;
        case INS_CROP_FLAG_13:
            type = ins::OffsetConvert::CONVERTTYPE::TITAN_5280_3956_5024_3380;
            break;
		default:
            LOGERR("no support crop flag:%d", crop_flag);
            return INS_ERR;
	}

    //LOGINFO("----corp flag:%d conv offset", crop_flag);
    ins::OffsetConvert::convertOffset(in, out, type);

    return INS_OK;
}