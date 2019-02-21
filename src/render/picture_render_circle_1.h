
#ifndef __PICTURE_RENDER_H__
#define __PICTURE_RENDER_H__

#include "render.h"
#include "picture_render_interface.h"
#include "insbuff.h"

class picture_render : public render, public picture_render_interface
{
public:
	virtual ~picture_render() override {};
	virtual int setup(int mode, int i_width, int i_height, int o_width, int o_height) override;
	virtual const unsigned char* draw(const std::vector<const unsigned char*>& image) override;

private:
	int setup_gles();
	int out_width_ = 0;
	int out_height_ = 0;
	std::shared_ptr<insbuff> pix_buff_;
};

#endif