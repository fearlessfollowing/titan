
#ifndef __PICTURE_RENDER_H__
#define __PICTURE_RENDER_H__

#include "render.h"
//#include "render_cube.h"
#include "insbuff.h"

class picture_render : public render
{
public:
	virtual ~picture_render() override {};
	int setup(int o_w, int o_h);
	const unsigned char* draw(const std::vector<const unsigned char*>& image, const float* mat);

private:
	int o_w_ = 0;
	int o_h_ = 0;
	std::shared_ptr<insbuff> pix_buff_;
};

#endif