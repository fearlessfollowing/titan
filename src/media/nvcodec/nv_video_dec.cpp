#include "nv_video_dec.h"
#include <unistd.h>
#include <assert.h>
#include "inslog.h"
#include "common.h"
#include "NvUtils.h"
#include "nvbuf_utils.h"
#include <linux/videodev2.h>

#define NV_CODED_BUFF_SIZE	1536*1024


/*
Example:
 static void abort(context_t *ctx)
 {
	 ctx->got_error = true;
	 ctx->dec->abort();
 
#ifndef USE_NVBUF_TRANSFORM_API
	 if (ctx->conv) {
		 ctx->conv->abort();
		 pthread_cond_broadcast(&ctx->queue_cond);	// 唤醒等待线程
	 }
#endif
 }
 */

#define NV_DEC_ABORT() \
    b_error_ = true; \
    if (dec_) dec_->abort();  \
    if (conv_) conv_->abort(); \
	LOGERR("%s abort", name_.c_str()); 



static bool conv_outputplane_cb(struct v4l2_buffer *v4l2_buf, NvBuffer *buffer, NvBuffer *shared_buffer, void *arg)
{
    assert(arg != nullptr);

    nv_video_dec* p = (nv_video_dec*)arg;

    return p->conv_output_plane_done(v4l2_buf, buffer, shared_buffer);
}



// static bool conv_captureplane_cb(struct v4l2_buffer *v4l2_buf,NvBuffer * buffer, NvBuffer * shared_buffer, void *arg)
// {
//     assert(arg != nullptr);

//     nv_video_dec* p = (nv_video_dec*)arg;

//     return p->conv_capture_plane_done(v4l2_buf, buffer, shared_buffer);
// }



void nv_video_dec::close()
{
    //NV_DEC_ABORT();

    if (th_.joinable()) th_.join();

    if (conv_) {
        if (conv_->output_plane.waitForDQThread(close_wait_time_ms_)) {
            LOGINFO("---%s wait:%u timeout conv abort", name_.c_str(), close_wait_time_ms_);
            conv_->abort();
            LOGINFO("---%s conv stop dq thread", name_.c_str());
            conv_->output_plane.stopDQThread();
            LOGINFO("---%s conv stop success", name_.c_str());
        }

        delete conv_;
        conv_ = nullptr;
    }

    if (dec_) {
        delete dec_;
        dec_ = nullptr;
    }

    LOGINFO("%s close", name_.c_str());
}




/***********************************************************************************************
** 函数名称: open
** 函数功能: 打开解码器(nv_video_dec)
** 入口参数:
**		msg 	 - 消息数据
**		cmd		 - 拍照命令"camera._takePicture"
**		sequence - 通信序列值
** 返 回 值: 成功返回INS_OK;失败返回错误码
*************************************************************************************************/
int32_t nv_video_dec::open(std::string name, std::string mime)
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

	/* 创建NV硬件解码器对象 */
    dec_ = NvVideoDecoder::createVideoDecoder(name.c_str());
    RETURN_IF_TRUE(dec_ == nullptr, "createVideoDecoder error", INS_ERR);

	/* 提交一个分辨率改变事件 */
    ret = dec_->subscribeEvent(V4L2_EVENT_RESOLUTION_CHANGE, 0, 0);
    RETURN_IF_TRUE(ret < 0, "subscribeEvent error", INS_ERR);

	/* 通知解码器支持非完整帧输入到解码器 */
    ret = dec_->disableCompleteFrameInputBuffer();
    RETURN_IF_TRUE(ret < 0, "disableCompleteFrameInputBuffer error", INS_ERR);

    ret = dec_->setOutputPlaneFormat(fmt, NV_CODED_BUFF_SIZE);
    RETURN_IF_TRUE(ret < 0, "setOutputPlaneFormat error", INS_ERR);

	/* Disabe Picture Buffer */
    ret = dec_->disableDPB();
    RETURN_IF_TRUE(ret < 0, "disableDPB error", INS_ERR);

	/*
	 * 修改一下mmap的数量(9 -> 6)
	 */
    ret = dec_->output_plane.setupPlane(V4L2_MEMORY_MMAP, 9, true, false);
    RETURN_IF_TRUE(ret < 0, "output_plane setupPlane error", INS_ERR);

	/* 构造转换器 */
    conv_ = NvVideoConverter::createVideoConverter("conv");
    RETURN_IF_TRUE(conv_ == nullptr, "createVideoConverter error", INS_ERR);

    ret = dec_->output_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "setStreamStatus error", INS_ERR);

    b_error_ = false;

	/*
	 * 创建解码采集loop线程
	 */
    th_ = std::thread(&nv_video_dec::capture_plane_loop, this);

    LOGINFO("%s open mime:%s", name_.c_str(), mime.c_str());

    return INS_OK;
}



int32_t nv_video_dec::setup_capture_plane()
{
    int ret = 0;
    struct v4l2_format format;
    struct v4l2_crop crop;

    /* 1.Get capture plane format from the decoder. This may change after
     * an resolution change event
     */
    ret = dec_->capture_plane.getFormat(format);
    RETURN_IF_TRUE(ret < 0, "getFormat error", INS_ERR);

	/*
	 * 2.Get the display resolution from the decoder
	 */
    ret = dec_->capture_plane.getCrop(crop);
    RETURN_IF_TRUE(ret < 0, "getCrop error", INS_ERR);
    LOGINFO("%s fmt change width:%d height:%d", name_.c_str(), crop.c.width, crop.c.height);


    /*
     * deinitPlane unmaps the buffers and calls REQBUFS with count 0 - test
     */
    dec_->capture_plane.deinitPlane();



    /* Not necessary to call VIDIOC_S_FMT on decoder capture plane.
     * But decoder setCapturePlaneFormat function updates the class variables
     */
    ret = dec_->setCapturePlaneFormat(format.fmt.pix_mp.pixelformat, 
										format.fmt.pix_mp.width, 
										format.fmt.pix_mp.height);
    RETURN_IF_TRUE(ret < 0, "setCapturePlaneFormat error", INS_ERR);


	/*
	 * Get the minimum buffers which have to be requested on the capture plane
	 */
    int min_buff = 0;
    ret = dec_->getMinimumCapturePlaneBuffers(min_buff);
    RETURN_IF_TRUE(ret < 0, "setCapturePlaneFormat error", INS_ERR);


	LOGINFO("--- getMinimumCapturePlaneBuffers: [%d]", min_buff);

    ret = dec_->capture_plane.setupPlane(V4L2_MEMORY_MMAP, 
											min_buff,		// min_buff -> min_buff + 5 
											false, 
											false);
    RETURN_IF_TRUE(ret < 0, "capure plain setupPlane error", INS_ERR);


    ret = conv_->setOutputPlaneFormat(format.fmt.pix_mp.pixelformat,
										format.fmt.pix_mp.width,
										format.fmt.pix_mp.height,
										V4L2_NV_BUFFER_LAYOUT_BLOCKLINEAR);
    RETURN_IF_TRUE(ret < 0, "conv setOutputPlaneFormat error", INS_ERR);

    /* nv编码器现在只支持V4L2_PIX_FMT_YUV420M,所以解码后的也固定为这个格式 */
    ret = conv_->setCapturePlaneFormat(V4L2_PIX_FMT_NV12M, 
    									crop.c.width, 
    									crop.c.height, 
    									V4L2_NV_BUFFER_LAYOUT_PITCH);
    RETURN_IF_TRUE(ret < 0, "conv setCapturePlaneFormat error", INS_ERR);


    ret = conv_->setCropRect(0, 0, crop.c.width, crop.c.height);
    RETURN_IF_TRUE(ret < 0, "conv setCropRect error", INS_ERR);

    // ret = conv_->setYUVRescale(V4L2_YUV_RESCALE_EXT_TO_STD);
    // RETURN_IF_TRUE(ret < 0, "conv setYUVRescale error", INS_ERR);


    ret = conv_->output_plane.setupPlane(V4L2_MEMORY_DMABUF, 
    									dec_->capture_plane.getNumBuffers(), 
    									false, 
    									false);
    RETURN_IF_TRUE(ret < 0, "conv output plane setupPlane error", INS_ERR);

    ret = conv_->capture_plane.setupPlane(V4L2_MEMORY_MMAP, 
											dec_->capture_plane.getNumBuffers(), 
											true, 
											false);
    RETURN_IF_TRUE(ret < 0, "conv capture plane setupPlane error", INS_ERR);


    ret = conv_->output_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "conv output plane setStreamStatus error", INS_ERR);
	
    ret = conv_->capture_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "conv capture plane setStreamStatus error", INS_ERR);


    for (uint32_t i = 0; i < conv_->output_plane.getNumBuffers(); i++) {
        conv_outp_queue_.push(conv_->output_plane.getNthBuffer(i));
    }

    for (uint32_t i = 0; i < conv_->capture_plane.getNumBuffers(); i++) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
		
        ret = conv_->capture_plane.qBuffer(v4l2_buf, nullptr);
        RETURN_IF_TRUE(ret < 0, "conv capture plane qBuffer error", INS_ERR);
    }

    //convert bl to pl for writing raw video to file
    conv_->output_plane.setDQThreadCallback(conv_outputplane_cb);
    //conv_->capture_plane.setDQThreadCallback(conv_captureplane_cb);

    conv_->output_plane.startDQThread(this);
    //conv_->capture_plane.startDQThread(this);


	/*
	 * Capture plane STREAMON
	 */
    ret = dec_->capture_plane.setStreamStatus(true);
    RETURN_IF_TRUE(ret < 0, "dec capture plane setStreamStatus error", INS_ERR);

	/*
	 * Enqueue all the empty capture plane buffers
	 */
    for (uint32_t i = 0; i < dec_->capture_plane.getNumBuffers(); i++) {
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
		
        v4l2_buf.index = i;
        v4l2_buf.m.planes = planes;
        ret = dec_->capture_plane.qBuffer(v4l2_buf, nullptr);
        RETURN_IF_TRUE(ret < 0, "dec capture plane qBuffer error", INS_ERR);
    }

    LOGINFO("Query and set capture successful[%s]", name_.c_str());
    return INS_OK;
}



void nv_video_dec::capture_plane_loop()
{
    int ret = 0;
    struct v4l2_event event;
    event.type = 0;
    
	LOGINFO("Starting decoder capture loop thread [%s]", name_.c_str());

	/*
	 * 1.等待一个V4L2_EVENT_RESOLUTION_CHANGE事件到来
	 */
	do {
        ret = dec_->dqEvent(event, 50000);	// 5000 -> 50000
        if (ret < 0) {
			if (errno == EAGAIN) {
				LOGERR("---> Timed out waiting for first V4L2_EVENT_RESOLUTION_CHANGE [%s]", name_.c_str());
			} else {
				LOGERR("---> Error in dequeueing decoder event [%s]", name_.c_str());
			}
			NV_DEC_ABORT();
            break;
        }

	} while (event.type != V4L2_EVENT_RESOLUTION_CHANGE && !b_eos_ && !b_error_);


	/*
	 * 2.分辨率事件改变后从新设置采集参数
	 */
    if (!b_eos_ && !b_error_) {
        if (setup_capture_plane() != INS_OK) {
            b_eos_ = true;
            b_error_ = true;
        }
    }

	/*
	 * 没有错误或没有接收到eos信号
	 */
    while (!(b_error_ || dec_->isInError() || b_eos_)) {

        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];
	
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;
        v4l2_buf.length = 3;		// demo中没有设置这一项??
        NvBuffer *dec_buffer;

		/*
		 * 从capture_plane中取出一帧已经解码的数据
		 */
        ret = dec_->capture_plane.dqBuffer(v4l2_buf, &dec_buffer, nullptr, 0);
        if (ret < 0) {
            if (errno == EAGAIN) {
                usleep(1000);
                continue;
            } else {
				LOGERR("capture_plane_loop: dqBuffer from dec capture_plane failed!\n");
                NV_DEC_ABORT();
                break;
            }
        }

        auto timestamp = v4l2_buf.timestamp;
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        memset(planes, 0, sizeof(planes));
        v4l2_buf.m.planes = planes;
        v4l2_buf.timestamp = timestamp;
        NvBuffer *conv_buffer = nullptr;

		/*
		 * 从转换器的输入Buffers队列中取出一个空闲的Buffer
		 */
        while (!(b_error_ || conv_->isInError() || b_eos_)) {
            if (!conv_outp_queue_.pop(conv_buffer)) {
                usleep(10*1000);
                continue;
            } else {
                v4l2_buf.index = conv_buffer->index;
                break;
            }
        }

        ret = conv_->output_plane.qBuffer(v4l2_buf, dec_buffer);
        if (ret < 0) {
			LOGERR("capture_plane_loop: dqBuffer buffer to conv's output_plane!\n");			
            NV_DEC_ABORT();
            break;
        }
    }


    conv_send_eos();

    LOGINFO("%s capture loop exit", name_.c_str());
}



void nv_video_dec::conv_send_eos()
{
    if (conv_ == nullptr) 
		return;

	/* 检查converter是否正在运行 */
    if (!conv_->output_plane.getStreamStatus()) {
		LOGINFO("---- converter is not running, skip conv_send_os");
		return;
    }

    NvBuffer *buff = nullptr;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[MAX_PLANES];
	
    memset(&v4l2_buf, 0, sizeof(v4l2_buf));
    memset(&planes, 0, sizeof(planes));
    v4l2_buf.m.planes = planes;


	/* 1.等待conv的
	 * 这个地方跟Demo的处理方式不一致
	 */
    int32_t retry = 15;
    while (--retry) {
        if (!conv_outp_queue_.pop(buff)) {
            usleep(20*1000);
            continue;
        } else {
            v4l2_buf.index = buff->index;
            break;
        }
    }


	/* 往转换器的output_plane中入队Eos buffer */
    if (retry) {
        /* Queue EOS buffer on converter output plane：byteuse = 0即为 eos */
        conv_->output_plane.qBuffer(v4l2_buf, nullptr);
        LOGINFO("%s conv send eos", name_.c_str());
    } else {
        LOGERR("%s conv cann't send eos", name_.c_str());
    }
}



bool nv_video_dec::conv_output_plane_done(struct v4l2_buffer *v4l2_buf,NvBuffer * buffer, NvBuffer * shared_buffer)
{   
    if (v4l2_buf == nullptr) {
        NV_DEC_ABORT();
        return false;
    }

    if (v4l2_buf->m.planes[0].bytesused == 0) {
        LOGINFO("%s conv output plane eos", name_.c_str());
        return false;//代表结束
    }

    struct v4l2_buffer dec_capture_buffer;
    struct v4l2_plane planes[MAX_PLANES];
    memset(&dec_capture_buffer, 0, sizeof(dec_capture_buffer));
    memset(planes, 0, sizeof(planes));
    dec_capture_buffer.index = shared_buffer->index;
    dec_capture_buffer.m.planes = planes;
    
    int ret = dec_->capture_plane.qBuffer(dec_capture_buffer, nullptr); 
    if (ret < 0) {
        NV_DEC_ABORT();
        return false;
    }

    conv_outp_queue_.push(buffer);

    return true;
}




/***********************************************************************************************
** 函数名称: dequeue_input_buff
** 函数功能: 从解码器的输入队列中取出一个空闲的NvBuffer
** 入口参数:
**		buff 	 - 空闲NvBuffer引用
**		timeout_ms - 等待的超时时间
** 返 回 值: 无
** 解码线程接收到模组的编码数据后,调用dequeue_input_buff来从解码器输入队列缓冲池中得到一个空闲
** 的V4L2 buffer
*************************************************************************************************/
int32_t nv_video_dec::dequeue_input_buff(NvBuffer* &buff, uint32_t timeout_ms)
{
    if (b_eos_ || b_error_) 
		return INS_ERR;

    if (dec_->isInError()) {
        LOGERR("%s in error", name_.c_str());
        b_error_ = true;
        return INS_ERR;
    }

    memset(&v4l2_buf_, 0, sizeof(v4l2_buf_));
    memset(planes_, 0, sizeof(planes_));
    v4l2_buf_.m.planes = planes_;

	/** 第一轮没有取完,直接获取空闲的NvBuffer */
    if (out_plane_index_ < dec_->output_plane.getNumBuffers()) {
        v4l2_buf_.index = out_plane_index_;
        buff = dec_->output_plane.getNthBuffer(out_plane_index_);
        out_plane_index_++;
        return INS_OK;
    } else {
		
		/* 队列中的NvBuffer都使用过一遍的情况下 */
        auto ret = dec_->output_plane.dqBuffer(v4l2_buf_, &buff, nullptr, timeout_ms);
        if (ret < 0) {
			LOGERR("dequeue_input_buff: dqBuffer failed, ret = %d", ret);

            if (errno == EAGAIN) 
				return INS_ERR_TIME_OUT;
			
            NV_DEC_ABORT();
            return INS_ERR;
        } else {
            return INS_OK;
        }
    }
}



/***********************************************************************************************
** 函数名称: queue_input_buff
** 函数功能: 将填充好数据的NvBuffer丢到解码器队列中
** 入口参数:
**		buff 	- 空闲NvBuffer引用
**		pts 	- 该帧数据的时间戳
** 返 回 值: 无
*************************************************************************************************/
void nv_video_dec::queue_input_buff(NvBuffer* buff, int64_t pts)
{
    if (b_eos_ || b_error_) return;

    if (buff == nullptr) {
        v4l2_buf_.m.planes[0].bytesused = 0;
    } else {
        v4l2_buf_.timestamp.tv_sec 		= pts / 1000000;
        v4l2_buf_.timestamp.tv_usec 	= pts % 1000000;
        v4l2_buf_.flags 				= V4L2_BUF_FLAG_TIMESTAMP_COPY;
        v4l2_buf_.m.planes[0].bytesused = buff->planes[0].bytesused;
    }

    auto ret = dec_->output_plane.qBuffer(v4l2_buf_, nullptr);
    if (ret < 0) {
		LOGINFO("--------------> queue_input_buff qBuffer failed");
        NV_DEC_ABORT();
        return;
    }

	/* 如果是最后一帧数据 */
    if (v4l2_buf_.m.planes[0].bytesused == 0) {
		
        //dequeue all output buffer when eos
        struct v4l2_buffer v4l2_buf;
        struct v4l2_plane planes[MAX_PLANES];

		/* 取出所有已入队列的的数据帧 */
        while (dec_->output_plane.getNumQueuedBuffers() > 0 && !dec_->isInError()) {
            memset(&v4l2_buf, 0, sizeof(v4l2_buf));
            memset(planes, 0, sizeof(planes));
            v4l2_buf.m.planes = planes;

            ret = dec_->output_plane.dqBuffer(v4l2_buf, nullptr, nullptr, -1);
            if (ret < 0) {
				LOGERR("---> Error DQing buffer at output plane");
                NV_DEC_ABORT();
                break;
            }
        }

        b_eos_ = true;
        LOGINFO("%s output plane send eos", name_.c_str());
    }
}



// void nv_video_dec::input_send_eos()
// {
//     NvBuffer* buff = nullptr;
//     if (INS_OK == dequeue_input_buff(buff, -1)) {
//         queue_input_buff(nullptr);
//     }
// }

int32_t nv_video_dec::dequeue_output_buff(NvBuffer* &buff, int64_t& pts, uint32_t timeout_ms)
{
    if (b_error_ || b_conv_eos_) 
		return INS_ERR;

    if (conv_->isInError()) {
        LOGERR("%s in error", name_.c_str());
        b_error_ = true;
        return INS_ERR;
    }

    memset(&v4l2_buf_conv_cap_, 0, sizeof(v4l2_buf_conv_cap_));
    memset(planes_conv_cap_, 0, sizeof(planes_conv_cap_));
    v4l2_buf_conv_cap_.m.planes = planes_conv_cap_;
    v4l2_buf_conv_cap_.length = 3;

    auto ret = conv_->capture_plane.dqBuffer(v4l2_buf_conv_cap_, &buff, nullptr, timeout_ms);
    if (ret < 0) {
        NV_DEC_ABORT();
        return INS_ERR;
    } else {
        if (v4l2_buf_conv_cap_.m.planes[0].bytesused == 0) {
            b_conv_eos_ = true;
            LOGINFO("%s conv capture plane eos", name_.c_str());
        }
        pts = v4l2_buf_conv_cap_.timestamp.tv_sec*1000000 + v4l2_buf_conv_cap_.timestamp.tv_usec;
        return INS_OK;
    }
}


void nv_video_dec::queue_output_buff(NvBuffer* buff)
{
    if (b_error_ || b_conv_eos_) 
		return;

    auto ret = conv_->capture_plane.qBuffer(v4l2_buf_conv_cap_, nullptr);
    if (ret < 0) {
        NV_DEC_ABORT();
    }
}


#if 0
bool nv_video_dec::conv_capture_plane_done(struct v4l2_buffer *v4l2_buf,NvBuffer *buffer, NvBuffer *shared_buffer)
{
	if (v4l2_buf == nullptr) {
		NV_DEC_ABORT();
		return false;
	}

	if (v4l2_buf->m.planes[0].bytesused == 0) return false;

	//do something
	int ret = conv_->capture_plane.qBuffer(*v4l2_buf, nullptr);
	if (ret < 0) {
		NV_DEC_ABORT();
		return false;
	}
	return true;
}
#endif

