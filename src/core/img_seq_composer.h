#ifndef _IMG_SEQ_COMPOSER_H_
#define _IMG_SEQ_COMPOSER_H_

#include <thread>
#include <vector>
#include "compose_option.h"

class nv_video_enc;
class nv_jpeg_dec;
class render;
class cam_img_repo;

class img_seq_composer
{
public:
    ~img_seq_composer();
    int32_t open(const compose_option& option);
    void set_repo(std::shared_ptr<cam_img_repo> repo) { repo_ = repo; };

private:
    void enc_task();
    int32_t compose(std::vector<int32_t>& v_in_fd, int32_t out_fd);
    std::thread th_;
    bool quit_ = false;
    uint32_t mode_ = 0;
    std::shared_ptr<nv_video_enc> enc_;
    std::vector<std::shared_ptr<nv_jpeg_dec>> dec_;
    std::shared_ptr<render> render_;
    std::shared_ptr<cam_img_repo> repo_;
    void* rend_img_ = nullptr;
};

#endif