#include "nv_video_enc.h"

#include <unistd.h>
//#include <errno.h>
#include <assert.h>

#include "inslog.h"
#include "common.h"

#include "NvUtils.h"
#include "nvbuf_utils.h"
#include <linux/videodev2.h>

#include "hw_util.h"

#define NV_CODED_BUFF_SIZE 2*1024*1024

#define NV_ENC_ABORT() \
    LOGERR("%s abort", name_.c_str()); \
    if (enc_) enc_->abort();  \
    b_error_ = true; \

static bool capture_plane_dq_cb(struct v4l2_buffer *v4l2_buf, NvBuffer *buffer, NvBuffer *shared_buffer, void *arg)
{
    assert(arg != nullptr);

    auto p = (nv_video_enc*)arg;

    return p->capture_plane_done(v4l2_buf, buffer, shared_buffer);
}

void nv_video_enc::close()
{
    if (enc_) {
        if (enc_->capture_plane.waitForDQThread(3000)) {	// 3s
            LOGERR("%s capture plane waitForDQThread fail", name_.c_str());
            enc_->abort();
            LOGERR("%s capture plane abort", name_.c_str());
            enc_->capture_plane.stopDQThread();
            LOGINFO("%s capture plane DQThread stop", name_.c_str());
        }
		
        delete enc_;
        enc_ = nullptr;
    }

    LOGINFO("%s close ", name_.c_str());
}


int32_t nv_video_enc::open(std::string name, std::string mime)
{
    int32_t ret = 0;
    name_ = name;
    mime_ = mime;

    uint32_t fmt;
    if (mime == INS_H264_MIME) {
        fmt = V4L2_PIX_FMT_H264;
    } else if (mime == INS_H265_MIME) {
        fmt = V4L2_PIX_FMT_H265;
    } else {
        LOGERR("unsupport mime:%s", mime.c_str()); 
        return INS_ERR;
    }

    enc_ = NvVideoEncoder::createVideoEncoder(name.c_str());
    RETURN_IF_TRUE(enc_ == nullptr, "createVideoEncoder error", INS_ERR);

    ret = enc_->setCapturePlaneFormat(fmt, width_, height_, NV_CODED_BUFF_SIZE);
    RETURN_IF_TRUE(ret < 0, "enc setCapturePlaneFormat error", INS_ERR);

    // V4L2_PIX_FMT_YUV420M
    ret = enc_->setOutputPlaneFormat(V4L2_PIX_FMT_YUV420M, width_, height_);
    RETURN_IF_TRUE(ret < 0, "enc setOutputPlaneFormat error", INS_ERR);

    ret = enc_->setBitrate(bitrate_);
    RETURN_IF_TRUE(ret < 0, "enc setBitrate error", INS_ERR);

    if (fmt == V4L2_PIX_FMT_H264) {
        ret = enc_->setProfile(V4L2_MPEG_VIDEO_H264_PROFILE_HIGH); 
        RETURN_IF_TRUE(ret < 0, "enc setProfile error", INS_ERR);
        if (width_*height_ >= 3200*3200)  { // 分辨率过大的时候， 必须自己设置level,不然编码器会自动设置为1,这样码率会很低
            ret = enc_->setLevel(V4L2_MPEG_VIDEO_H264_LEVEL_5_1); 
        }
        RETURN_IF_TRUE(ret < 0, "enc setLevel error", INS_ERR);
    }

    ret = enc_->setRateControlMode(V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
    RETURN_IF_TRUE(ret < 0, "enc setRateControlMode error", INS_ERR);

    ret = enc_->setPeakBitrate(bitrate_*1.2);
    RETURN_IF_TRUE(ret < 0, "enc setPeakBitrate error", INS_ERR);

    uint32_t interval = (int32_t)framerate_.to_double();
    if (interval < 1) interval = 1;
    ret = enc_->setIDRInterval(interval); 
    RETURN_IF_TRUE(ret < 0, "enc setIDRInterval error", INS_ERR);

    ret = enc_->setIFrameInterval(interval);
    RETURN_IF_TRUE(ret < 0, "enc setIFrameInterval error", INS_ERR);

    ret = enc_->setFrameRate(framerate_.num, framerate_.den);
    RETURN_IF_TRUE(ret < 0, "enc setFrameRate error", INS_ERR);

    ret = enc_->setHWPresetType(V4L2_ENC_HW_PRESET_FAST);  //V4L2_ENC_HW_PRESET_FAST
    RETURN_IF_TRUE(ret < 0, "enc setHWPresetType error", INS_ERR);

    ret = enc_->setInsertSpsPpsAtIdrEnabled(true);
    RETURN_IF_TRUE(ret < 0, "enc setInsertSpsPpsAtIdrEnabled error", INS_ERR);

    ret = enc_->setNumReferenceFrames(1);
    RETURN_IF_TRUE(ret < 0, "enc setNumReferenceFrames error", INS_ERR);

    // ret = enc_->setInsertVuiEnabled(true);
    // RETURN_IF_TRUE(ret < 0, "enc setInsertVuiEnabled error", INS_ERR);
    
    // ret = enc_->setNumBFrames(0);
    // RETURN_IF_TRUE(ret < 0, "enc setNumBFrames error", INS_ERR);

    //ret = enc_->setTemporalTradeoff(V4L2_ENC_TEMPORAL_TRADEOFF_LEVEL_DROPNONE);
    //RETURN_IF_TRUE(ret < 0, "enc setTemporalTradeoff error", INS_ERR);

    //ret = enc_->setSliceLength(V4L2_ENC_SLICE_LENGTH_TYPE_MBLK, 1024);
    //RETURN_IF_TRUE(ret < 0, "enc setSliceLength error", INS_ERR);

    //ret = enc_->setVirtualBufferSize(???);
    //RETURN_IF_TRUE(ret < 0, "enc setVirtualBufferSize error", INS_ERR);

    // ret = enc_->setSliceIntrarefresh(ctx.slice_intrarefresh_interval);
    // RETURN_IF_TRUE(ret < 0, "enc setSliceIntrarefresh error", INS_ERR);

    // ret = enc_->setQpRange(ctx.nMinQpI, ctx.nMaxQpI, ctx.nMinQpP, ctx.nMaxQpP, ctx.nMinQpB, ctx.nMaxQpB);
    // RETURN_IF_TRUE(ret < 0, "enc setQpRange error", INS_ERR);

    ret = enc_->output_plane.setupPlane(V4L2_MEMORY_MMAP, 9, true, false);
    RETURN_IF_TRUE(ret < 0, "enc output plane setupPlane error", INS_ERR);

    ret = enc_->capture_plane.setupPlane(V4L2_MEMORY_MMAP, 9, true, false);
    RETURN_IF_TRUE(ret < 0, "enc capture plane setupPlane error", INS_ERR);

    ret = enc_->output_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "enc output plane setStreamStatus error", INS_ERR);

    ret = enc_->capture_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "enc capture plane setStreamStatus error", INS_ERR);

    enc_->capture_plane.setDQThreadCallback(capture_plane_dq_cb);
    enc_->capture_plane.startDQThread(this);

    for (uint32_t i = 0; i < enc_->capture_plane.getNumBuffers(); i++) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));
        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
        ret = enc_->capture_plane.qBuffer(v4l2_buf, nullptr);
        if (ret < 0) {
            NV_ENC_ABORT();
            return INS_ERR;
        }
    }

    pool_ = std::make_shared<obj_pool<ins_frame>>(-1, "nvenc");

    b_error_ = false;

    LOGINFO("%s open mime:%s w:%u h:%u bitrate:%u fps:%d/%d", name_.c_str(), mime.c_str(), width_, height_, bitrate_, framerate_.num, framerate_.den);

    return INS_OK;
}

bool nv_video_enc::capture_plane_done(struct v4l2_buffer *v4l2_buf, NvBuffer *buffer, NvBuffer *shared_buffer)
{
    if (v4l2_buf == nullptr) {
        NV_ENC_ABORT();
        return false; 
    }

    if (buffer->planes[0].bytesused == 0) {
        LOGINFO("%s capture plane eos", name_.c_str());
        return false;
    }
    
    bool b_keyframe = v4l2_buf->flags & V4L2_BUF_FLAG_KEYFRAME;
    int64_t pts = v4l2_buf->timestamp.tv_sec*1000*1000 + v4l2_buf->timestamp.tv_usec;
    output_frame(buffer, pts, b_keyframe);

    if (enc_->capture_plane.qBuffer(*v4l2_buf, nullptr)) {
        NV_ENC_ABORT();
        return false; 
    }

    return true;
}


int32_t nv_video_enc::dequeue_input_buff(NvBuffer* &buff, uint32_t timeout_ms)
{
    if (b_error_ || b_eos_) return INS_ERR;

    if (enc_->isInError()) {
        LOGERR("%s is in error", name_.c_str());
        b_error_ = true;
        return INS_ERR;
    }

    memset(&v4l2_buf_, 0, sizeof(v4l2_buf_));
    memset(planes_, 0, sizeof(planes_));
    v4l2_buf_.m.planes = planes_;

    if (out_plane_index_ < enc_->output_plane.getNumBuffers()) {
        v4l2_buf_.index = out_plane_index_;
        buff = enc_->output_plane.getNthBuffer(out_plane_index_);
        out_plane_index_++;
        return INS_OK;
    } else {
        auto ret = enc_->output_plane.dqBuffer(v4l2_buf_, &buff, nullptr, timeout_ms);
        if (ret < 0) {
            if (errno == EAGAIN) return INS_ERR_TIME_OUT;
            NV_ENC_ABORT();
            return INS_ERR;
        } else {
            return INS_OK;
        }
    }
}

int32_t nv_video_enc::queue_intput_buff(NvBuffer* buff, int64_t pts)
{
    if (b_error_ || b_eos_) return INS_OK;

    //buff null： send eos
    if (buff == nullptr) {
        v4l2_buf_.m.planes[0].bytesused = 0;
    } else {
        v4l2_buf_.timestamp.tv_sec = pts/1000000;
        v4l2_buf_.timestamp.tv_usec = pts%1000000;
        v4l2_buf_.flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
        //LOGINFO("%s pts:%lld timestamp:%u %u", name_.c_str(), pts, v4l2_buf_.timestamp.tv_sec, v4l2_buf_.timestamp.tv_usec);
        v4l2_buf_.m.planes[0].bytesused = buff->planes[0].bytesused;

        //write_video_frame(buff); 
    }

    int ret = enc_->output_plane.qBuffer(v4l2_buf_, nullptr);
    if (ret < 0) { 
        LOGERR("%s output plane qbuffer fail:%d error:%d %s", name_.c_str(), ret, errno, strerror(errno));
        // LOGINFO("-----cpu:%d GPU:%d", hw_util::get_cpu_temp(), hw_util::get_gpu_temp());
        // system("cat /proc/meminfo >> /home/nvidia/mem.txt");
        // system("cat /proc/meminfo >> /home/nvidia/mem.txt");
        NV_ENC_ABORT();
        return INS_ERR;
    }
    
    if (v4l2_buf_.m.planes[0].bytesused == 0) {
        LOGINFO("%s output plain send eos", name_.c_str());
        b_eos_ = true;
    }

    return INS_OK;
}

void nv_video_enc::send_eos()
{
    NvBuffer* buff = nullptr;
    if (INS_OK == dequeue_input_buff(buff, -1)) {
        queue_intput_buff(nullptr, 0);
    }
}

void nv_video_enc::output_frame(NvBuffer* buffer, int64_t pts, bool b_keyframe)
{
    if ((buffer->planes[0].data[4] & 0x1f) == 7) {
        buffer->planes[0].data[6] = 0; // h264规定该filed必须为0，不为0的时候像AS3等播放器可能播放不了
    }

    if (sps_pps_ == nullptr) {
        sps_pps_ = std::make_shared<insbuff>(buffer->planes[0].bytesused);
        memcpy(sps_pps_->data(), buffer->planes[0].data, sps_pps_->size());
        LOGINFO("%s sps pps size:%d", name_.c_str(), buffer->planes[0].bytesused);
    } else {
        auto frame = pool_->pop();
        //auto frame = std::make_shared<ins_frame>();
        frame->page_buf = std::make_shared<page_buffer>(buffer->planes[0].bytesused);
        memcpy(frame->page_buf->data(), buffer->planes[0].data, buffer->planes[0].bytesused);
        frame->media_type = INS_MEDIA_VIDEO;
        frame->pts = frame->dts = pts;
        frame->is_key_frame = b_keyframe;
        frame->duration = 1000000.0/framerate_.to_double();

        std::lock_guard<std::mutex> lock(mtx_);
        for (auto it = m_sink_.begin(); it != m_sink_.end(); it++) {
            if (!it->second->is_video_param_set()) {
                if (b_keyframe && sps_pps_ != nullptr) {
                    ins_video_param param;
                    param.mime = mime_;
                    param.width = width_;
                    param.height = height_;
                    param.fps = framerate_.to_double();
                    param.bitrate = bitrate_/1000; 
                    param.v_timebase = 1000000;
                    param.config = sps_pps_;
                    it->second->set_video_param(param);
                } else {
                    return;
                }
            }

            it->second->queue_frame(frame);
        }
    }
}

// void nv_video_enc::write_video_frame(NvBuffer* buffer)
// {
//     uint32_t i, j;
//     char *data;

//     if (fp_ == nullptr)
//     {
//         std::string name = "/mnt/udisk1/" + name_ + ".yuv";
//         fp_ = fopen(name.c_str(), "wb");
//     }

//     for (i = 0; i < buffer->n_planes; i++)
//     {
//         NvBuffer::NvBufferPlane &plane = buffer->planes[i];
//         size_t bytes_to_write = plane.fmt.bytesperpixel * plane.fmt.width;

//         // char* p = (char *) plane.data;
//         // for (j = 0; j < plane.fmt.height; j++)
//         // {
//         //     for (int k = 0; k < plane.fmt.width; k++)
//         //     {
//         //         if ((p[k] < 16) || (i == 0 && p[k] > 235) || (i != 0 && p[k] > 240))
//         //         {
//         //             printf("plane:%d value:%d\n",i, p[k]);
//         //         }
//         //         if (i == 0)
//         //         {
//         //             p[k] = 16 + 219*p[k]/255; 
//         //         }
//         //         else
//         //         {
//         //             p[k] = 16 + 224*p[k]/255; 
//         //         }
//         //     }
//         //     p += plane.fmt.stride;
//         // }

//         data = (char *) plane.data;
//         for (j = 0; j < plane.fmt.height; j++)
//         {
//             fwrite(data, 1, bytes_to_write, fp_);
//             data += plane.fmt.stride;
//         }
//     }
// }

