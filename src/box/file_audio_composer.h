#ifndef _FILE_AUDIO_COMPOSER_H_
#define _FILE_AUDIO_COMPOSER_H_

#include <memory>
#include "sink_interface.h"
#include "insdemux.h"

class spatial_audio_composer;

class file_audio_composer
{
public:
    void set_param(const ins_demux_param& param);
    void queue_frame(int index, const std::shared_ptr<ins_demux_frame>& frame);
    int open(std::shared_ptr<sink_interface>& sink, bool b_compose, bool b_in_spatial);

private:
    std::shared_ptr<sink_interface> sink_;
    std::shared_ptr<spatial_audio_composer> spatial_composer_;
    bool b_in_spatial_ = false;
};

#endif
