
#include "ffutil.h"
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/error.h"
#include "libavcodec/avcodec.h"
}

void ffutil::init()
{   
    av_register_all();
    avformat_network_init();
}

std::string ffutil::err2str(int err)
{
	char str[AV_ERROR_MAX_STRING_SIZE] = {0};
	return std::string(av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, err));
}