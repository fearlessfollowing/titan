#ifndef _TIFF_TAG_H_
#define _TIFF_TAG_H_

#include <string>

struct tiff_tags
{
	uint8_t dng_version[4] = {0};
	uint8_t dng_bw_version[4] = {0};
	std::string make;
	std::string model;
	std::string uniquecameramodel;
	std::string software;
	std::string datetime;

	uint32_t width;
	uint32_t height;
	uint32_t subfiletype;
	uint16_t compression;
	uint16_t bitpersample;
	uint16_t sampleperpixel;
	uint16_t rowperstrip;
	uint16_t plannarconfig;
	uint16_t orientation;
	uint16_t photometric;
	uint16_t samplefmt;
	uint32_t whitelevel;
	float blacklevel;
	int16_t cfalayout;
	int16_t cfarepeatpatterndim[2];
	uint8_t cfapattern[4];
	uint8_t cfaplanecolor[3];
	float asshotneutral[3];
	float colormatrix1[9];
	int16_t calibrationilluminant1;

	uint16_t sharpness = 0;
	uint16_t contrast = 0;
	uint16_t saturation = 0;
	uint16_t wb = 0;
	uint16_t exposure_program = 0;
	uint16_t metering_mode = 0;
	float fnumber = 0;
	float iso_speed = 0;
	float exposure_time = 0;
	float exposure_bias_value = 0;
	float focal_length = 0;
	float shutter_speed = 0;
};

#endif
