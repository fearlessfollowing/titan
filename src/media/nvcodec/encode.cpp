#include <iostream>
#include <fstream>
#include <string.h>
#include <thread>
#include <mutex>
#include <memory>
#include <deque>
#include <chrono>

#include <unistd.h>
#include <errno.h>

#include "inslog.h"
#include "ffutil.h"
#include "insdemux.h"

#include "NvUtils.h"
#include "nvbuf_utils.h"
#include <linux/videodev2.h>
#include "NvVideoEncoder.h"
#include "NvVideoConverter.h"
#include "NvEglRenderer.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "NvCudaProc.h"

using namespace std::chrono;

NvVideoEncoder* _enc = nullptr;
bool _error = false;
std::ofstream _out_file;
std::ifstream _in_file;
int _in_frames = 0;
int _out_frames = 0;
std::deque<char*> _queue;
std::mutex _mtx;
int width = 0;
int height = 0;
EGLDisplay _egl_display = nullptr;

#define RETURN_IF_TRUE(cond, str, ret) \
if (cond) \
{ \
    LOGERR("%s", str); \
    return ret; \
} 

#define BREAK_ABORT_IF_TRUE(cond, str) \
if (cond) \
{ \
    LOGERR("%s abort", str); \
    if (_enc) _enc->abort();  \
    _error = true; \
    break; \
}

#define RETURN_ABORT_IF_TRUE(cond, str, ret) \
if (cond) \
{ \
    LOGERR("%s abort", str); \
    if (_enc) _enc->abort();  \
    _error = true; \
    return ret; \
}

char* dequeue_frame()
{
    std::lock_guard<std::mutex> lock(_mtx);
    if (_queue.empty()) return nullptr;
    auto buff = _queue.front();
    _queue.pop_front();
    return buff;
}

void read_frame_loop()
{
    while (1)
    {
        if (_queue.size() > 100) break;

        uint32_t size = width*height*3/2;
        auto buff = new char[size];
        _in_file.read(buff, size);
        if (_in_file.gcount() < size)
        {
            LOGINFO("in file read over");
            delete[] buff;
            break;
        }

        _mtx.lock();
        _queue.push_back(buff);
        _mtx.unlock();
    }

    LOGINFO("read frame loop exit");
}

int read_video_frame(NvBuffer* buffer)
{
    //printf("queue size:%lu\n", _queue.size());

    auto data = dequeue_frame();
    if (data == nullptr)
    {
        LOGINFO("frame queue is null");
        return -1;
    }

    uint32_t offset = 0;
    for (int32_t i = 0; i < buffer->n_planes; i++)
    {
        auto &plane = buffer->planes[i];

        auto p = (char*)plane.data;
        auto line_size = plane.fmt.bytesperpixel*plane.fmt.width;

        for (int32_t j = 0; j < plane.fmt.height; j++)
        {
            memcpy(p, data+offset, line_size);
            offset += line_size;
            p += plane.fmt.stride;
        }
        plane.bytesused = plane.fmt.stride * plane.fmt.height;
    }

    if (data) delete[] data;

    _in_frames++;

    auto eglimg = NvEGLImageFromFd(_egl_display, buffer->planes[0].fd);
    HandleEGLImage(&eglimg);
    NvDestroyEGLImage(_egl_display, eglimg);

    return 0;
}

// int read_video_frame(NvBuffer* buffer)
// {
//     //if (_in_frames > 50) return -1;

//     for (int32_t i = 0; i < buffer->n_planes; i++)
//     {
//         auto &plane = buffer->planes[i];
//         auto line_size = plane.fmt.bytesperpixel * plane.fmt.width;
//         auto data = (char*) plane.data;
//         plane.bytesused = 0;

//         for (int32_t j = 0; j < plane.fmt.height; j++)
//         {
//             _in_file.read(data, line_size);
//             if (_in_file.gcount() < line_size)
//             {
//                 LOGINFO("in file read over");
//                 return -1;
//             }
//             data += plane.fmt.stride;
//         }
//         plane.bytesused = plane.fmt.stride * plane.fmt.height;
//     }

//     _in_frames++;

//     return 0;
// }

void write_video_frame(NvBuffer* buffer)
{
    if (_out_file.is_open())
    {
        _out_file.write((char*)buffer->planes[0].data, buffer->planes[0].bytesused);
        if (!_out_file.good())
        {
            LOGERR("out file write error");
        }
    }

    _out_frames++;
}

bool enc_capture_plane_dq_cb(struct v4l2_buffer *v4l2_buf, NvBuffer * buffer, NvBuffer * shared_buffer, void *arg)
{
    RETURN_ABORT_IF_TRUE(v4l2_buf == nullptr, "enc cb v4l2_buf is null", false);

    if (buffer->planes[0].bytesused == 0)
    {
        LOGINFO("enc cb recevie buff bytesused 0");
        return false;
    }

    write_video_frame(buffer);

    int ret = _enc->capture_plane.qBuffer(*v4l2_buf, nullptr);
    RETURN_ABORT_IF_TRUE(ret < 0, "enc capture plane qbuffer error", false);

    return true;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int bitrate = 40*1024*1024;
    uint32_t fmt;
    std::string in_file_name;
    std::string out_file_name;

    int ch, opt_cnt = 0;
    while ((ch = getopt(argc, argv, "w:h:i:o:f:")) != -1)
    {
        switch (ch)
        {
            case 'w':
                width = atoi(optarg);
                opt_cnt++;
                break;
            case 'h':
                height = atoi(optarg);
                opt_cnt++;
                break;
            case 'f':
                printf("fmt opt:%s\n", optarg);
                if (std::string("h265") == std::string(optarg))
                {
                    fmt = V4L2_PIX_FMT_H265;
                }
                else
                {
                    fmt = V4L2_PIX_FMT_H264;
                }
                opt_cnt++;
                break;
            case 'i':
                in_file_name = optarg;
                opt_cnt++;
                break;
            case 'o':
                out_file_name = optarg;
                opt_cnt++;
                break;
            case 'm':
                printf("option: w:h:i:o:f:\n");
                return 0;
            default:
                break;
        }

    }

    if (opt_cnt < 5)
    {
        printf("option: w:h:i:o:f:\n");
        return -1;
    }

    ins_log::init("./", "log");

    LOGINFO("w:%d h:%d in file:%s out file:%s", width, height, in_file_name.c_str(), out_file_name.c_str());

    _egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    RETURN_IF_TRUE(_egl_display == EGL_NO_DISPLAY, "eglGetDisplay error", -1);

    ret = eglInitialize(_egl_display, nullptr, nullptr);
    RETURN_IF_TRUE(ret == 0, "eglInitialize error", -1);

    _out_file.open(out_file_name);
    RETURN_IF_TRUE(!_out_file.is_open(), "out file open error", -1);

    _in_file.open(in_file_name);
    RETURN_IF_TRUE(!_in_file.is_open(), "in file open error", -1);

    read_frame_loop();

    _enc = NvVideoEncoder::createVideoEncoder("enc0");
    RETURN_IF_TRUE(_enc == nullptr, "createVideoEncoder error", -1);

    ret = _enc->setCapturePlaneFormat(fmt, width, height, 3*1024*1024);
    RETURN_IF_TRUE(ret < 0, "enc setCapturePlaneFormat error", -1);

    ret = _enc->setOutputPlaneFormat(V4L2_PIX_FMT_YUV420M, width, height);
    RETURN_IF_TRUE(ret < 0, "enc setOutputPlaneFormat error", -1);

    ret = _enc->setBitrate(bitrate);
    RETURN_IF_TRUE(ret < 0, "enc setBitrate error", -1);

    if (fmt == V4L2_PIX_FMT_H264)
    {
        ret = _enc->setProfile(V4L2_MPEG_VIDEO_H264_PROFILE_HIGH);
        RETURN_IF_TRUE(ret < 0, "enc setProfile error", -1);

        ret = _enc->setLevel(V4L2_MPEG_VIDEO_H264_LEVEL_5_1);
        RETURN_IF_TRUE(ret < 0, "enc setLevel error", -1);
    }

    ret = _enc->setRateControlMode(V4L2_MPEG_VIDEO_BITRATE_MODE_VBR);
    RETURN_IF_TRUE(ret < 0, "enc setRateControlMode error", -1);

    ret = _enc->setIDRInterval(30);
    RETURN_IF_TRUE(ret < 0, "enc setIDRInterval error", -1);

    ret = _enc->setIFrameInterval(30);
    RETURN_IF_TRUE(ret < 0, "enc setIFrameInterval error", -1);

    ret = _enc->setFrameRate(30, 1);
    RETURN_IF_TRUE(ret < 0, "enc setFrameRate error", -1);

    ret = _enc->setHWPresetType(V4L2_ENC_HW_PRESET_FAST);
    RETURN_IF_TRUE(ret < 0, "enc setHWPresetType error", -1);

    ret = _enc->setInsertSpsPpsAtIdrEnabled(true);
    RETURN_IF_TRUE(ret < 0, "enc setInsertSpsPpsAtIdrEnabled error", -1);

    ret = _enc->setNumReferenceFrames(1);
    RETURN_IF_TRUE(ret < 0, "enc setNumReferenceFrames error", -1);

    // ret = _enc->setNumBFrames(0);
    // RETURN_IF_TRUE(ret < 0, "enc setNumBFrames error", -1);

    //ret = _enc->setTemporalTradeoff(V4L2_ENC_TEMPORAL_TRADEOFF_LEVEL_DROPNONE);
    //RETURN_IF_TRUE(ret < 0, "enc setTemporalTradeoff error", -1);

    //ret = _enc->setSliceLength(V4L2_ENC_SLICE_LENGTH_TYPE_MBLK, 1024);
    //RETURN_IF_TRUE(ret < 0, "enc setSliceLength error", -1);

    //ret = _enc->setVirtualBufferSize(???);
    //RETURN_IF_TRUE(ret < 0, "enc setVirtualBufferSize error", -1);

    // ret = _enc->setSliceIntrarefresh(ctx.slice_intrarefresh_interval);
    // RETURN_IF_TRUE(ret < 0, "enc setSliceIntrarefresh error", -1);

    // ret = _enc->setQpRange(ctx.nMinQpI, ctx.nMaxQpI, ctx.nMinQpP, ctx.nMaxQpP, ctx.nMinQpB, ctx.nMaxQpB);
    // RETURN_IF_TRUE(ret < 0, "enc setQpRange error", -1);

    ret = _enc->output_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false);
    RETURN_IF_TRUE(ret < 0, "enc output plane setupPlane error", -1);

    ret = _enc->capture_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false);
    RETURN_IF_TRUE(ret < 0, "enc capture plane setupPlane error", -1);

    ret = _enc->output_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "enc output plane setStreamStatus error", -1);

    ret = _enc->capture_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "enc capture plane setStreamStatus error", -1);

    _enc->capture_plane.setDQThreadCallback(enc_capture_plane_dq_cb);
    _enc->capture_plane.startDQThread(nullptr);

    LOGINFO("enc out && capture plane setup");

    auto start = steady_clock::now(); 

    bool eos = false;

    for (uint32_t i = 0; i < _enc->capture_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));
        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
        ret = _enc->capture_plane.qBuffer(v4l2_buf, nullptr);
        BREAK_ABORT_IF_TRUE(ret < 0, "enc capture plane qbuffer error");
    }

    for (uint32_t i = 0; i < _enc->output_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, MAX_PLANES * sizeof(struct v4l2_plane));
        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;

        NvBuffer *buffer = _enc->output_plane.getNthBuffer(i);

        if (read_video_frame(buffer) < 0)
        {
            v4l2_buf.m.planes[0].bytesused = 0;
        }

        ret = _enc->output_plane.qBuffer(v4l2_buf, nullptr);
        BREAK_ABORT_IF_TRUE(ret < 0, "enc output plane qbuffer error");

        if (v4l2_buf.m.planes[0].bytesused == 0)
        {
            LOGINFO("input file read over");
            eos = true; break;
        }
    }

    LOGINFO("out put plane all enqueue");

    while (!_error && !_enc->isInError() && !eos)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;

        NvBuffer *buffer;
        ret = _enc->output_plane.dqBuffer(v4l2_buf, &buffer, nullptr, 10);
        BREAK_ABORT_IF_TRUE(ret < 0, "enc output plane dqBuffer error");

        if (read_video_frame(buffer) < 0)
        {
            v4l2_buf.m.planes[0].bytesused = 0;
        }

        ret = _enc->output_plane.qBuffer(v4l2_buf, NULL);
        BREAK_ABORT_IF_TRUE(ret < 0, "enc output plane qbuffer error");

        if (v4l2_buf.m.planes[0].bytesused == 0) 
        {
            LOGINFO("input file read over");
            eos = true; break;   
        }
    }

    _enc->capture_plane.waitForDQThread(-1);

    auto end = steady_clock::now();
    auto t = duration_cast<duration<double>>(end-start);

    LOGINFO("-----------frames:%d %d time:%lf fps:%lf", _in_frames, _out_frames, t.count(), _in_frames/t.count());

    delete _enc;
    _out_file.close();

    LOGINFO("main exit");
}