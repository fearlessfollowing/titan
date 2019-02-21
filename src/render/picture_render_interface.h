
#ifndef __PICTURE_RENDER_INTERFACE_H__
#define __PICTURE_RENDER_INTERFACE_H__

#include <vector>

class picture_render_interface
{
public:
	virtual ~picture_render_interface(){};
	virtual int setup(int mode, int i_width, int i_height, int o_width, int o_height)=0;
	virtual const unsigned char* draw(const std::vector<const unsigned char*>& image, const float* mat)=0;
};

#endif