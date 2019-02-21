#include "audio_sender.h"
#include "common.h"
#include "inslog.h"
#include "fifo_write.h"

int32_t audio_sender::open()
{
    sender_ = std::make_shared<fifo_write>();
	sender_->start("/home/nvidia/insta360/fifo/audio");

    LOGINFO("--------audio sender open");
    return INS_OK;
}

void audio_sender::queue_frame(const ins_pcm_frame& frame)
{
    sender_->queue_buff(frame->buff);
}