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
#include "NvVideoDecoder.h"
#include "NvVideoConverter.h"
#include "NvEglRenderer.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"
#include "NvCudaProc.h"

using namespace std::chrono;

NvVideoDecoder* _dec = nullptr;
NvVideoConverter* _conv = nullptr;
NvEglRenderer* _render = nullptr;
EGLDisplay _egl_display = nullptr;
std::shared_ptr<InsDemux> _demux;
bool _error = false;
bool _eos = false;
std::deque<NvBuffer*> _queue;
std::mutex _queue_mtx;
std::ofstream out_file;
int _frames = 0;
int _frames_w = 0;

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
    if (_dec) _dec->abort();  \
    if (_conv) _conv->abort(); \
    _error = true; \
    break; \
}

#define RETURN_ABORT_IF_TRUE(cond, str, ret) \
if (cond) \
{ \
    LOGERR("%s abort", str); \
    if (_dec) _dec->abort();  \
    if (_conv) _conv->abort(); \
    _error = true; \
    return ret; \
}

void read_frame(NvBuffer* buffer)
{
    buffer->planes[0].bytesused = 0;

    std::shared_ptr<InsDemuxFrame> frame;
    while (1)
    {
        if (_demux->GetNextFrame(frame))
        {
            LOGINFO("file read over");
            break;
        }

        if (frame->media_type == INS_MEDIA_VIDEO)
        {
            _frames++;
            memcpy(buffer->planes[0].data, frame->data, frame->len);
            buffer->planes[0].bytesused = frame->len;
            break;
        }
    }
}

void write_frame(NvBuffer* buffer)
{
    if (out_file.is_open())
    {
        for (uint32_t i = 0; i < buffer->n_planes; i++)
        {
            NvBuffer::NvBufferPlane &plane = buffer->planes[i];
            auto line_size = plane.fmt.bytesperpixel * plane.fmt.width;
            
            auto data = (char*)plane.data;
            for (uint32_t j = 0; j < plane.fmt.height; j++)
            {
                out_file.write(data, line_size);
                if (!out_file.good())
                {
                    LOGERR("file write fail");
                    return;
                }
                data += plane.fmt.stride;
            }
        }

        _frames_w++;
        printf("write frame:%d\n", _frames_w);
    }
}

NvBuffer* dequeue_buff()
{
    std::lock_guard<std::mutex> lock(_queue_mtx);
    if (_queue.empty()) return nullptr;
    
    auto buff = _queue.front();
    _queue.pop_front();

    return buff;
}

void queue_buff(NvBuffer* buff)
{
    std::lock_guard<std::mutex> lock(_queue_mtx);
    _queue.push_back(buff);
}

bool conv_outputplane_cb(struct v4l2_buffer *v4l2_buf,NvBuffer * buffer, NvBuffer * shared_buffer, void *arg)
{   
    RETURN_ABORT_IF_TRUE(v4l2_buf == nullptr, "conv_outputplane_cb v4l2_buf is null", false);

    if (v4l2_buf->m.planes[0].bytesused == 0) return false;

    struct v4l2_buffer dec_capture_buffer;
    struct v4l2_plane planes[MAX_PLANES];
    memset(&dec_capture_buffer, 0, sizeof(dec_capture_buffer));
    memset(planes, 0, sizeof(planes));
    dec_capture_buffer.index = shared_buffer->index;
    dec_capture_buffer.m.planes = planes;

    int ret = _dec->capture_plane.qBuffer(dec_capture_buffer, nullptr); 
    RETURN_ABORT_IF_TRUE(ret < 0, "conv_outputplane_cb dec capture plane qBuffer error", false);

    queue_buff(buffer);

    return true;
}

bool conv_captureplane_cb(struct v4l2_buffer *v4l2_buf,NvBuffer * buffer, NvBuffer * shared_buffer, void *arg)
{
    RETURN_ABORT_IF_TRUE(v4l2_buf == nullptr, "conv_captureplane_cb v4l2_buf is null", false);

    if (v4l2_buf->m.planes[0].bytesused == 0) return false;

    // if (_render) _render->render(buffer->planes[0].fd);

    // auto egl_image = NvEGLImageFromFd(_egl_display, buffer->planes[0].fd);
    // RETURN_IF_TRUE(egl_image == nullptr, "NvEGLImageFromFd error", false);

    // // Running algo process with EGLImage via GPU multi cores
    // HandleEGLImage(&egl_image);

    // NvDestroyEGLImage(_egl_display, egl_image);

    write_frame(buffer);

    int ret = _conv->capture_plane.qBuffer(*v4l2_buf, nullptr);
    RETURN_ABORT_IF_TRUE(ret < 0, "conv_outputplane_cb conv capture plane qBuffer error", false);

    return true;
}

int setup_capture_plane()
{
    int ret = 0;
    struct v4l2_format format;
    struct v4l2_crop crop;

    ret = _dec->capture_plane.getFormat(format);
    RETURN_IF_TRUE(ret < 0, "getFormat error", -1);
    char* p = (char*)&format.fmt.pix_mp.pixelformat;
    printf("pixfmt:%c %c %c %c w:%d h:%d\n", p[0],p[1],p[2],p[3], format.fmt.pix_mp.width,format.fmt.pix_mp.height);

    ret = _dec->capture_plane.getCrop(crop);
    RETURN_IF_TRUE(ret < 0, "getCrop error", -1);
    LOGINFO("width:%d height:%d", crop.c.width, crop.c.height);

    //_render = NvEglRenderer::createEglRenderer("render0", crop.c.width, crop.c.height, 0, 0);
    //_render->setFPS(30);

    ret = _dec->setCapturePlaneFormat(format.fmt.pix_mp.pixelformat,format.fmt.pix_mp.width,format.fmt.pix_mp.height);
    RETURN_IF_TRUE(ret < 0, "setCapturePlaneFormat error", -1);

    int min_buff = 0;
    ret = _dec->getMinimumCapturePlaneBuffers(min_buff);
    RETURN_IF_TRUE(ret < 0, "setCapturePlaneFormat error", -1);

    ret = _dec->capture_plane.setupPlane(V4L2_MEMORY_MMAP,min_buff + 5, false, false);
    RETURN_IF_TRUE(ret < 0, "capure plain setupPlane error", -1);

    //conv
    ret = _conv->setOutputPlaneFormat(format.fmt.pix_mp.pixelformat,format.fmt.pix_mp.width,format.fmt.pix_mp.height,V4L2_NV_BUFFER_LAYOUT_BLOCKLINEAR);
    RETURN_IF_TRUE(ret < 0, "conv setOutputPlaneFormat error", -1);

    ret = _conv->setCapturePlaneFormat(V4L2_PIX_FMT_YUV420M,crop.c.width,crop.c.height,V4L2_NV_BUFFER_LAYOUT_PITCH);
    RETURN_IF_TRUE(ret < 0, "conv setCapturePlaneFormat error", -1);

    ret = _conv->setCropRect(0, 0, crop.c.width, crop.c.height);
    RETURN_IF_TRUE(ret < 0, "conv setCapturePlaneFormat error", -1);

    ret = _conv->output_plane.setupPlane(V4L2_MEMORY_DMABUF, _dec->capture_plane.getNumBuffers(), false, false);
    RETURN_IF_TRUE(ret < 0, "conv output plane setupPlane error", -1);

    ret = _conv->capture_plane.setupPlane(V4L2_MEMORY_MMAP, _dec->capture_plane.getNumBuffers(), true, false);
    RETURN_IF_TRUE(ret < 0, "conv capture plane setupPlane error", -1);

    ret = _conv->output_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "conv output plane setStreamStatus error", -1);

    ret = _conv->capture_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "conv capture plane setStreamStatus error", -1);

    for (uint32_t i = 0; i < _conv->output_plane.getNumBuffers(); i++)
    {
        queue_buff(_conv->output_plane.getNthBuffer(i));
    }

    //conv capture plane fill empty buffer
    for (uint32_t i = 0; i < _conv->capture_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));

        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
        ret = _conv->capture_plane.qBuffer(v4l2_buf, nullptr);
        RETURN_IF_TRUE(ret < 0, "conv capture plane qBuffer error", -1);
    }

    _conv->output_plane.startDQThread(nullptr);
    _conv->capture_plane.startDQThread(nullptr);

    LOGINFO("conv output & capture plane setup");

    ret = _dec->capture_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "dec capture plane setStreamStatus error", -1);

    // dev capture plane fill all empty buffer
    for (uint32_t i = 0; i < _dec->capture_plane.getNumBuffers(); i++)
    {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));

        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
        ret = _dec->capture_plane.qBuffer(v4l2_buf, nullptr);
        RETURN_IF_TRUE(ret < 0, "dec capture plane qBuffer error", -1);
    }

    LOGINFO("dec capture plane setup");

    return 0;
}

void dec_capture_loop()
{
    int ret = 0;
    struct v4l2_event event;
    event.type = 0;
    
    LOGINFO("dec capure loop start");

    while (event.type != V4L2_EVENT_RESOLUTION_CHANGE)
    {
        int ret = _dec->dqEvent(event, 50000);
        BREAK_ABORT_IF_TRUE(ret < 0, "dqEvent error");
    }

    if (!_error) setup_capture_plane();

    //deque dec capture plane then fill conv output plane
    while (!(_error || _dec->isInError() || _eos))
    {
        while (1)
        {
            struct v4l2_buffer v4l2_buf;
            struct v4l2_plane planes[MAX_PLANES];
            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            memset(planes, 0, sizeof(planes));
            v4l2_buf.m.planes = planes;

            NvBuffer *dec_buffer;
            NvBuffer *conv_buffer;

            ret = _dec->capture_plane.dqBuffer(v4l2_buf, &dec_buffer, nullptr, 0);
            if (ret < 0)
            {
                if (errno == EAGAIN)
                {
                    LOGINFO("dec capture plane dqbuffer eagain");
                    usleep(1000); break;
                }
                else
                {
                    BREAK_ABORT_IF_TRUE(true, "dec capture plane dqBuffer error");
                }
            }

            // auto egl_image = NvEGLImageFromFd(_egl_display, dec_buffer->planes[0].fd);
            // if (egl_image == nullptr) abort();
            // HandleEGLImage(&egl_image);
            // NvDestroyEGLImage(_egl_display, egl_image);

            //if (_render) _render->render(dec_buffer->planes[0].fd);

            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            memset(planes, 0, sizeof(planes));
            v4l2_buf.m.planes = planes;

            while (1)
            {
                conv_buffer = dequeue_buff();
                if (conv_buffer == nullptr)
                {
                    usleep(10*1000);
                    continue;
                }
                else
                {
                    v4l2_buf.index = conv_buffer->index;
                    break;
                }
            }

            ret = _conv->output_plane.qBuffer(v4l2_buf, dec_buffer);
            BREAK_ABORT_IF_TRUE(ret < 0, "conv output plane qBuffer error");
        }
    }

    LOGINFO("dec capture plane finish");

    //conv send eos
    if (_conv->output_plane.getStreamStatus())
    {
        LOGINFO("conv send eos");

        NvBuffer *conv_buffer;
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(&planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;

        while (1)
        {
            conv_buffer = dequeue_buff();
            if (conv_buffer == nullptr)
            {
                usleep(10*1000);
                continue;
            }
            else
            {
                v4l2_buf.index = conv_buffer->index;
                break;
            }
        }

        // Queue EOS buffer on converter output plane：byteuse = 0即为 eos
        _conv->output_plane.qBuffer(v4l2_buf, nullptr);
    }

    LOGINFO("capture loop exit");
}

int main(int argc, const char* argv[])
{
    int ret = 0;

    ins_log::init("./", "log");

    RETURN_IF_TRUE(argc < 2, "please input filename", -1);
    
    ffutil::init();

    _egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    RETURN_IF_TRUE(_egl_display == EGL_NO_DISPLAY, "eglGetDisplay error", -1);

    ret = eglInitialize(_egl_display, nullptr, nullptr);
    RETURN_IF_TRUE(ret == 0, "eglInitialize error", -1);

    _demux = std::make_shared<InsDemux>();
    ret = _demux->Open(argv[1]);
    RETURN_IF_TRUE(ret, "demux open error", -1);

    _dec = NvVideoDecoder::createVideoDecoder("dec0");
    RETURN_IF_TRUE(_dec == nullptr, "createVideoDecoder error", -1);

    ret = _dec->subscribeEvent(V4L2_EVENT_RESOLUTION_CHANGE, 0, 0);
    RETURN_IF_TRUE(ret < 0, "subscribeEvent error", -1);

    ret = _dec->disableCompleteFrameInputBuffer();
    RETURN_IF_TRUE(ret < 0, "disableCompleteFrameInputBuffer error", -1);

    ret = _dec->setOutputPlaneFormat(V4L2_PIX_FMT_H264, 4000000);
    RETURN_IF_TRUE(ret < 0, "setOutputPlaneFormat error", -1);

    ret = _dec->disableDPB();
    RETURN_IF_TRUE(ret < 0, "disableDPB error", -1);

    ret = _dec->output_plane.setupPlane(V4L2_MEMORY_MMAP, 10, true, false);
    RETURN_IF_TRUE(ret < 0, "output_plane setupPlane error", -1);

    _conv = NvVideoConverter::createVideoConverter("conv0");
    RETURN_IF_TRUE(_conv == nullptr, "createVideoConverter error", -1);

    //convert bl to pl for writing raw video to file
    _conv->output_plane.setDQThreadCallback(conv_outputplane_cb);
    _conv->capture_plane.setDQThreadCallback(conv_captureplane_cb);

    ret = _dec->output_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "setStreamStatus error", -1);   

    LOGINFO("dec output plane start");

    out_file.open("./out.yuv");
    RETURN_IF_TRUE(!out_file.is_open(), "file open error", -1);  

    auto start = steady_clock::now(); 
    
    //start loop
    std::thread th = std::thread(dec_capture_loop);

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[MAX_PLANES];
    NvBuffer *buffer;
    int index = 0;
    bool eos = false;

    //fill all empty output plane
    while (!_dec->isInError() && index < _dec->output_plane.getNumBuffers())
    {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;

        buffer = _dec->output_plane.getNthBuffer(index);
        read_frame(buffer);
        v4l2_buf.index = index++;
        v4l2_buf.m.planes[0].bytesused = buffer->planes[0].bytesused;

        ret = _dec->output_plane.qBuffer(v4l2_buf, nullptr);
        BREAK_ABORT_IF_TRUE(ret < 0, "output_plane qBuffer error");

        if (v4l2_buf.m.planes[0].bytesused == 0) 
        {
            eos = true; break;
        }
    }

    //dequeue output plane then enqueue
    while (!_dec->isInError() && !eos && !_error) 
    {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;

        ret = _dec->output_plane.dqBuffer(v4l2_buf, &buffer, nullptr, -1);
        BREAK_ABORT_IF_TRUE(ret < 0, "output_plane dqBuffer error");

        read_frame(buffer);
        v4l2_buf.m.planes[0].bytesused = buffer->planes[0].bytesused;
        
        ret = _dec->output_plane.qBuffer(v4l2_buf, nullptr);
        BREAK_ABORT_IF_TRUE(ret < 0, "output_plane qBuffer error");

        if (v4l2_buf.m.planes[0].bytesused == 0) 
        {
            eos = true; break;
        }
    }

    //dequeue all left output plane
    while (_dec->output_plane.getNumQueuedBuffers() > 0 && !_dec->isInError() && !_error)
    {
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;

        ret = _dec->output_plane.dqBuffer(v4l2_buf, nullptr, nullptr, -1);
        if (ret < 0) _error = true;
        BREAK_ABORT_IF_TRUE(ret < 0, "output_plane dqBuffer error");
    }

    LOGINFO("dec output plane finish");

    _eos = true;

    if (th.joinable()) th.join();

    _conv->capture_plane.waitForDQThread(-1);

    auto end = steady_clock::now();
    auto t = duration_cast<duration<double>>(end - start);
    LOGINFO("---------time:%lf frames:%d %d fps:%lf", t.count(), _frames, _frames_w, (double)_frames/t.count());

    delete _dec;
    delete _conv;
    delete _render;

    if (_egl_display) eglTerminate(_egl_display);

    out_file.close();
    _queue.clear();
    _demux = nullptr;

    LOGINFO("main exit");

    return 0;
}