
#include <nv_video_dec.h>
#include "inslog.h"
#include "common.h"
#include "ffutil.h"
#include "insdemux.h"
#include <unistd.h>
#include <chrono>

using namespace std::chrono;

std::shared_ptr<InsDemux> _demux;
std::shared_ptr<nv_video_dec> _dec;
FILE* _fp = nullptr;
uint32_t _frames_r = 0;
uint32_t _frames_w = 0;

int32_t read_frame(NvBuffer* buffer)
{
    std::shared_ptr<InsDemuxFrame> frame;
    while (1)
    {
        if (_demux->GetNextFrame(frame))
        {
            //LOGINFO("file read over");
            return INS_ERR;
        }

        if (frame->media_type == INS_MEDIA_VIDEO)
        {
            memcpy(buffer->planes[0].data, frame->data, frame->len);
            buffer->planes[0].bytesused = frame->len;
            _frames_r++;
            return INS_OK;
        }
    }
}

void write_frame(NvBuffer* buffer)
{
    _frames_w++;
    //printf("frame:%d\n", _frames_w);

    if (_fp == nullptr) return;

    for (uint32_t i = 0; i < buffer->n_planes; i++)
    {
        NvBuffer::NvBufferPlane &plane = buffer->planes[i];
        auto line_size = plane.fmt.bytesperpixel * plane.fmt.width;
        
        auto data = (char*)plane.data;
        for (uint32_t j = 0; j < plane.fmt.height; j++)
        {
            auto ret = fwrite(data, 1, line_size, _fp);
            if (ret != line_size)
            {
                LOGERR("file write fail");
                return;
            }
            data += plane.fmt.stride;
        }
    }
}

void output_loop()
{
    NvBuffer* buff_out = nullptr;
    while (1)
    {
        auto ret = _dec->dequeue_output_buff(buff_out, 0);
        if (ret != INS_OK)
        {
            //LOGINFO("output error");
            break;
        }
        else
        {
            write_frame(buff_out);
            _dec->queue_output_buff(buff_out);
        }
    }

    LOGINFO("main output loop exit");
}

int32_t main(int32_t argc, char* argv[])
{
    std::string i_file_name;
    std::string o_file_name;
    int32_t ret = 0;

    int32_t ch, opt_cnt = 0;
    while ((ch = getopt(argc, argv, "i:o:")) != -1)
    {
        switch (ch)
        {
            case 'i':
                i_file_name = optarg;
                opt_cnt++;
                break;
            case 'o':
                o_file_name = optarg;
                opt_cnt++;
                break;
            default:
                break;
        }
    }

    if (opt_cnt < 2)
    {
        printf("option: i:o:\n");
        return 0;
    }

    ins_log::init("./", "log");
    ffutil::init();

    // _fp = fopen(o_file_name.c_str(), "wb");
    // RETURN_IF_TRUE(_fp == nullptr, "out file open fail", -1);

    _demux = std::make_shared<InsDemux>();
    ret = _demux->Open(i_file_name.c_str());
    RETURN_IF_TRUE(ret, "demux open error", ret);

    InsDemuxParam demux_param;
    _demux->VideoAudioParam(demux_param);

    _dec = std::make_shared<nv_video_dec>();
    ret = _dec->open("dec", "h264");
    RETURN_IF_TRUE(ret, "decode open fail", ret);

    std::thread th = std::thread(output_loop);

    auto start_time = steady_clock::now(); 

    NvBuffer* buff_in = nullptr;
    while (1)
    {
        ret = _dec->dequeue_input_buff(buff_in, 20);
        if (ret == INS_ERR_TIME_OUT)
        {
            continue;
        }
        else if (ret != INS_OK)
        {
            break;
        }
        else
        {
            ret = read_frame(buff_in);
            if (ret != INS_OK) buff_in->planes[0].bytesused = 0;
            _dec->queue_input_buff(buff_in);
            if (ret != INS_OK) break;
        }
    }

    LOGINFO("main input loop exit");

    th.join();

    // if (_fp != nullptr) 
    // {
    //     fclose(_fp);
    //     _fp = nullptr;
    // }

    auto end_time = steady_clock::now(); 
    auto d = duration_cast<duration<double>>(end_time-start_time);

    LOGINFO("---------frames:%u %u fps:%lf", _frames_r, _frames_w, _frames_r/d.count());

    _demux = nullptr;

    _dec = nullptr;

    return 0;
}