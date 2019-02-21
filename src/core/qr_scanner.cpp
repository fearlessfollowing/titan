
#include "qr_scanner.h"
#include "inslog.h"
#include "common.h"
#include "access_msg_center.h"
#include <sstream>
#include "zbar.h"
#include "Dewarp/Dewarp.h"
#include "xml_config.h"
#include "cam_manager.h"
#include "nv_video_dec.h"
#include "NvBuffer.h"
#include "insbuff.h"
#include "one_cam_video_buff.h"
#include "shared_ptr_array.h"
#include "ins_clock.h"

using namespace zbar;

#define QR_WIDTH   2560  //注意: 如果width不是256倍数，要增加对齐处理
#define QR_HEIGHT  1920
#define QR_FR      15
#define QR_BITRATE 30000
#define OUT_WIDTH  720
#define OUT_HEIGHT 720

qr_scanner::~qr_scanner()
{
    quit_ = true;

    for (uint32_t i = 0; i < dec_.size(); i++)
    {
        INS_THREAD_JOIN(th_dec_[i]);
    }

    for (uint32_t i = 0; i < 4; i++)
    {
        INS_THREAD_JOIN(th_scan_[i]);
    }

    if (camera_) 
    {
        camera_->stop_all_video_rec();
        camera_ = nullptr;
    }

    LOGINFO("qr scanner close");
}

int32_t qr_scanner::open(std::shared_ptr<cam_manager> camera)
{ 
    camera_ = camera; 
    std::map<int32_t,std::shared_ptr<cam_video_buff_i>> m_video_buff;
    for (int32_t i = 0; i < INS_CAM_NUM; i++)
    {
        if (i != 0 && i != INS_CAM_NUM-1) continue; //扫码只用两个镜头
        auto queue = std::make_shared<one_cam_video_buff>();
        repo_.push_back(queue);
        m_video_buff.insert(std::make_pair(i, queue));
    }

    cam_video_param param;
	param.width = QR_WIDTH;
	param.height = QR_HEIGHT;
	param.framerate = QR_FR;
	param.bitrate = QR_BITRATE;
    param.b_file_stream = false; 
	param.b_usb_stream = true;

    auto ret = camera_->set_all_video_param(param, nullptr); 
	RETURN_IF_NOT_OK(ret);

	ret = camera_->start_all_video_rec(m_video_buff);
    RETURN_IF_NOT_OK(ret);

    for (uint32_t i = 0; i < repo_.size(); i++)
	{
        auto dec = std::make_shared<nv_video_dec>();
	    auto ret = dec->open("dec", "h264");
	    RETURN_IF_NOT_OK(ret);
        dec_.push_back(dec);
        dec_exit_.push_back(false);
    }

    std::string offset = xml_config::get_offset(INS_CROP_FLAG_4_3);
    ins::Dewarp map;
    map.setOffset(offset);
    std::vector<int32_t> v_width(INS_CAM_NUM, QR_WIDTH);
    std::vector<int32_t> v_height(INS_CAM_NUM, QR_HEIGHT);
    map.init(v_width, v_height, OUT_WIDTH, OUT_HEIGHT);
    for (uint32_t i = 0; i < 2; i++)
    {
        int32_t index = i?(INS_CAM_NUM-1):0;
        float* uv_get = map.getCubeMap(index, ins::CUBETYPE::BACK, ins::MAPTYPE::CPU, 1);
        auto uv = make_shared_array<float>(OUT_WIDTH*OUT_HEIGHT*sizeof(float)*2);
        memcpy(uv.get(), uv_get, OUT_WIDTH*OUT_HEIGHT*sizeof(float)*2);
        uv_.push_back(uv);
    }

    for (uint32_t i = 0; i < 2; i++) //zbar解码太慢，两个线程同时zbar解码
    {
        th_scan_[i] = std::thread(&qr_scanner::scan_task, this, 0);
    }

    for (uint32_t i = 0; i < 2; i++) //zbar解码太慢，两个线程同时zbar解码
    {
        th_scan_[i+2] = std::thread(&qr_scanner::scan_task, this, 1);
    }

    for (uint32_t i = 0; i < dec_.size(); i++)
    {
        th_dec_[i] = std::thread(&qr_scanner::dec_task, this, i);
    }

    LOGINFO("qr scan open success");

    return INS_OK;
}

void qr_scanner::dec_task(uint32_t index)
{
    int ret;
	int64_t pts;
	NvBuffer* buff = nullptr;
    while (1)
    {
        ret = dec_[index]->dequeue_input_buff(buff, 20);
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
            ret = dequeue_frame(buff, pts, index);
            if (ret != INS_OK) buff->planes[0].bytesused = 0;
            dec_[index]->queue_input_buff(buff, pts);
            if (ret != INS_OK) break;
        }
    }

    dec_exit_[index] = true;

	LOGINFO("dec:%u task exit", index);
}

void qr_scanner::scan_task(uint32_t index)
{
    bool ret = false;
    auto y = std::make_shared<insbuff>(OUT_WIDTH*OUT_HEIGHT);

    zbar_image_scanner_t* scanner = zbar_image_scanner_create();
    zbar_image_scanner_set_config(scanner, (zbar_symbol_type_t)0, ZBAR_CFG_ENABLE, 1);  
    zbar_image_t* image = zbar_image_create();
    zbar_image_set_format(image, v4l2_fourcc('Y','8','0','0')); //灰度图
    zbar_image_set_size(image, OUT_HEIGHT, OUT_HEIGHT);

    int64_t pts;
	NvBuffer* buff;
	while (!quit_ || !dec_exit_[index])
	{
        mtx_[index].lock();
        auto ret = dec_[index]->dequeue_output_buff(buff, pts, 0);
        mtx_[index].unlock();
		BREAK_IF_NOT_OK(ret);

        if (!ret)
        {
            ins::Dewarp::getImage(buff->planes[0].data, y->data(), uv_[index].get(), QR_WIDTH, QR_HEIGHT, OUT_WIDTH, OUT_HEIGHT, 1);
            zbar_image_set_data(image, y->data(), y->size(), nullptr);
            zbar_scan_image(scanner, image);
            ret = parse_scann_result(image);
            if (index == 0) print_fps_info();
        }

        mtx_[index].lock();
        dec_[index]->queue_output_buff(buff);
        mtx_[index].unlock();
    }

    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner); 

    LOGINFO("scan task:%d exit", index);
}

bool qr_scanner::parse_scann_result(void* img)
{
    zbar_image_t* image = (zbar_image_t*)img;
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) 
    {
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        if (ZBAR_QRCODE != typ) continue; //扫到非QR码不算
        
        const char *data = zbar_symbol_get_data(symbol);
        LOGINFO("decode symbol:%s", data);

        std::unique_ptr<char[]> buff(new char[(strlen(data)+1)*3]());
        auto p = buff.get();
        for (unsigned int i = 0; i < strlen(data); i++)
        {
            sprintf(p+i*3, "%02x ", data[i]);
        }

        json_obj obj;
        obj.set_string("name", INTERNAL_CMD_QR_SCAN_FINISH);
        obj.set_string("content", p);
        access_msg_center::queue_msg(0, obj.to_string());

        return true;
    }

    return false;
}

int32_t qr_scanner::dequeue_frame(NvBuffer* buff, int64_t& pts, uint32_t index)
{
    while (!quit_)
    {
        auto frame = repo_[index]->deque_frame();
        if (frame == nullptr)
        {
            usleep(5*1000);
            continue;
        }
        else
        {
            memcpy(buff->planes[0].data, frame->page_buf->data(), frame->page_buf->offset());
            buff->planes[0].bytesused = frame->page_buf->offset();
            pts = frame->pts;
            return INS_OK;
        }
    }
    return INS_ERR;
}

void qr_scanner::print_fps_info() const
{
	static int cnt = -1;
	static struct timeval start_time;
	static struct timeval end_time;

	if (cnt == -1)
	{
		gettimeofday(&start_time, nullptr);
	}

	if (cnt++ > 60)
	{
		gettimeofday(&end_time, nullptr);
		double fps = 1000000.0*cnt/(double)(end_time.tv_sec*1000000 + end_time.tv_usec - start_time.tv_sec*1000000 - start_time.tv_usec);
		start_time = end_time;
		cnt = 0;
		LOGINFO("fps: %lf", fps);
	}
}