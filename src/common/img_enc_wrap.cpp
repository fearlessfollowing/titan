#include "img_enc_wrap.h"
#include "tjpegenc.h"
#include "common.h"
#include "inslog.h"
#include "jpeg_muxer.h"
extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}

img_enc_wrap::img_enc_wrap(int32_t mode, int32_t map, const jpeg_metadata* metadata): map_(map), mode_(mode)
{
    if (metadata) {
        M = std::make_shared<jpeg_metadata>();
        *(M.get()) = *metadata;
        M->b_gyro = false;
        M->b_spherical = true; 
        M->map_type =  map_;
    }
}


int32_t img_enc_wrap::encode(const ins_img_frame& frame, std::vector<std::string> url)
{
    tjpeg_enc enc;
    int32_t ret = enc.open(TJPF_BGR, 0);
    RETURN_IF_NOT_OK(ret);

    ins_img_frame enc_frame;
    ret = enc.encode(frame, enc_frame);
    RETURN_IF_NOT_OK(ret);

    if (M) M->width = frame.w;
    if (M) M->height = frame.h;
    jpeg_muxer muxer;
    muxer.create(url[0], enc_frame.buff->data(), enc_frame.buff->size(), M.get());
    
    if ((mode_ == INS_MODE_3D_TOP_LEFT || mode_ == INS_MODE_3D_TOP_RIGHT)) {
        if (url.size() < 3) return INS_OK;

        std::string url_1, url_2;
        if (mode_ == INS_MODE_3D_TOP_LEFT) {
            url_1 = url[1];
            url_2 = url[2];
        } else {
            url_1 = url[2];
            url_2 = url[1];
        }
        
        if (M) M->height /= 2;
        ins_img_frame in, out;
        in.w = frame.w;
        in.h = frame.h/2;
        in.buff = frame.buff;
        ret = enc.encode(in, out);
        RETURN_IF_NOT_OK(ret);
    
	    muxer.create(url_1, out.buff->data(), out.buff->size(), M.get());

        in.buff = std::make_shared<insbuff>(frame.buff->data() + frame.w*frame.h*3/2, frame.w*frame.h*3/2, false);
        ret = enc.encode(in, out);
        RETURN_IF_NOT_OK(ret);
        muxer.create(url_2, out.buff->data(), out.buff->size(), M.get());
    }

    return INS_OK;
}

int32_t img_enc_wrap::create_thumbnail(const ins_img_frame& frame, std::string url)
{
    int32_t th_w, th_h;
    if (map_ == INS_MAP_CUBE) {
		th_w = INS_THUMBNAIL_WIDTH_CUBE;
		th_h = INS_THUMBNAIL_HEIGHT_CUBE;
	} else {
		th_w = INS_THUMBNAIL_WIDTH;
		th_h = INS_THUMBNAIL_HEIGHT;
    }

    int32_t h = frame.h;
    int32_t w = frame.w;
    if (mode_ != INS_MODE_PANORAMA) h /= 2;
    
	AVPicture i_pic;
    memset(&i_pic, 0x00, sizeof(AVPicture));
	i_pic.data[0] = frame.buff->data();
	i_pic.linesize[0] = w*3;

	AVPicture o_pic; 
	avpicture_alloc(&o_pic, AV_PIX_FMT_BGR24, th_w, th_h);

	auto sw_ctx = sws_getContext(w, h, AV_PIX_FMT_BGR24, th_w, th_h, AV_PIX_FMT_BGR24,SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (sw_ctx == nullptr) {
		LOGERR("sws_getContext fail");
		return INS_ERR;
	}

	/*
	 * 先缩放,再编码
	 */
	sws_scale(sw_ctx, i_pic.data, i_pic.linesize, 0, h, o_pic.data, o_pic.linesize);

	tjpeg_enc enc;
    int32_t ret = enc.open(TJPF_BGR, 0);
    RETURN_IF_NOT_OK(ret);
    ins_img_frame in, out;
    in.w = th_w;
    in.h = th_h;
    in.buff = std::make_shared<insbuff>(o_pic.data[0], th_w*th_h*3, false);
	ret = enc.encode(in, out);
    RETURN_IF_NOT_OK(ret);
	
	avpicture_free(&o_pic);
	sws_freeContext(sw_ctx);

    if (M) M->width = th_w;
    if (M) M->height = th_h;
    jpeg_muxer muxer;
	ret = muxer.create(url, out.buff->data(), out.buff->size(), M.get());

    return ret;
}


