#ifdef __AUDIO_SENDER_H__
#define __AUDIO_SENDER_H__

#include <stdint.h>
#include <memory>
#include "ins_frame.h"

class fifo_write;

class audio_sender
{
public:
    int32_t open();
    void queue_frame(const ins_pcm_frame& frame);
private:
    std::shared_ptr<fifo_write> sender_;
};

#endif