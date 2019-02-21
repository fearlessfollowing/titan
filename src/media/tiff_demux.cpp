#include "tiff_demux.h"
#include "tiffio.h"
#include "common.h"
#include "inslog.h"

int tiff_demux::parse(std::string filename, ins_img_frame& frame, tiff_tags& tags)
{
    auto tif = TIFFOpen(filename.c_str(), "r");
    if (!tif)
    {
        LOGERR("tiff open file:%s fail", filename.c_str());
        return INS_ERR;
    }

    char* value;
    uint32_t value_cnt;

    //info
    TIFFGetField(tif, TIFFTAG_DNGVERSION, &value);
    memcpy(tags.dng_version, value, sizeof(tags.dng_version));
    TIFFGetField(tif, TIFFTAG_DNGBACKWARDVERSION, &value);
    memcpy(tags.dng_bw_version, value, sizeof(tags.dng_bw_version));
    TIFFGetField(tif, TIFFTAG_MAKE, &value);
    tags.make = value;
    TIFFGetField(tif, TIFFTAG_MODEL, &value);
    tags.model = value;
    TIFFGetField(tif, TIFFTAG_UNIQUECAMERAMODEL, &value);
    tags.uniquecameramodel = value;
    TIFFGetField(tif, TIFFTAG_SOFTWARE, &value);
    tags.software = value;
    TIFFGetField(tif, TIFFTAG_DATETIME, &value);
    tags.datetime = value;

    //img param
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &tags.width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &tags.height);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &tags.bitpersample);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &tags.sampleperpixel);
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &tags.compression);
    TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &tags.rowperstrip);
    TIFFGetField(tif, TIFFTAG_SUBFILETYPE, &tags.subfiletype);
    TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &tags.plannarconfig);
    TIFFGetField(tif, TIFFTAG_ORIENTATION, &tags.orientation);
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &tags.photometric);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &tags.samplefmt);
    TIFFGetField(tif, TIFFTAG_WHITELEVEL, &value_cnt, &value);
    memcpy(&tags.whitelevel, value, sizeof(tags.whitelevel));
    TIFFGetField(tif, TIFFTAG_BLACKLEVEL, &value_cnt, &value);
    memcpy(&tags.blacklevel, value, sizeof(tags.blacklevel));
    TIFFGetField(tif, TIFFTAG_CFALAYOUT, &tags.cfalayout);
    TIFFGetField(tif, TIFFTAG_CALIBRATIONILLUMINANT1, &tags.calibrationilluminant1);
    TIFFGetField(tif, TIFFTAG_CFAREPEATPATTERNDIM, &value);
    memcpy(tags.cfarepeatpatterndim, value, sizeof(tags.cfarepeatpatterndim));
    TIFFGetField(tif, TIFFTAG_CFAPATTERN, &value);
    memcpy(tags.cfapattern, value, sizeof(tags.cfapattern));
    TIFFGetField(tif, TIFFTAG_CFAPLANECOLOR, &value_cnt, &value);
    memcpy(tags.cfaplanecolor, value, sizeof(tags.cfaplanecolor));
    TIFFGetField(tif, TIFFTAG_ASSHOTNEUTRAL, &value_cnt, &value);
    memcpy(tags.asshotneutral, value, sizeof(tags.asshotneutral));
    TIFFGetField(tif, TIFFTAG_COLORMATRIX1, &value_cnt, &value);	
    memcpy(tags.colormatrix1, value, sizeof(tags.colormatrix1));

    //exif
    TIFFGetField(tif, EXIFTAG_EXPOSUREPROGRAM, &value_cnt, &value);
	memcpy(&tags.exposure_program, value, sizeof(tags.exposure_program));
	TIFFGetField(tif, EXIFTAG_METERINGMODE, &value_cnt, &value);
	memcpy(&tags.metering_mode, value, sizeof(tags.metering_mode));
	TIFFGetField(tif, EXIFTAG_WHITEBALANCE, &value_cnt, &value);
	memcpy(&tags.wb, value, sizeof(tags.wb));
	TIFFGetField(tif, EXIFTAG_CONTRAST, &value_cnt, &value);
	memcpy(&tags.contrast, value, sizeof(tags.contrast));
	TIFFGetField(tif, EXIFTAG_SATURATION, &value_cnt, &value);
	memcpy(&tags.saturation, value, sizeof(tags.saturation));
	TIFFGetField(tif, EXIFTAG_SHARPNESS, &value_cnt, &value);
    memcpy(&tags.sharpness, value, sizeof(tags.sharpness));
    TIFFGetField(tif, EXIFTAG_EXPOSURETIME, &value_cnt, &value);
    memcpy(&tags.exposure_time, value, sizeof(tags.exposure_time));
    TIFFGetField(tif, EXIFTAG_FNUMBER, &value_cnt, &value);
    memcpy(&tags.fnumber, value, sizeof(tags.fnumber));
    TIFFGetField(tif, EXIFTAG_ISOSPEEDRATINGS, &value_cnt, &value);
    memcpy(&tags.iso_speed, value, sizeof(tags.iso_speed));
    TIFFGetField(tif, EXIFTAG_EXPOSUREBIASVALUE, &value_cnt, &value);
    memcpy(&tags.exposure_bias_value, value, sizeof(tags.exposure_bias_value));
    TIFFGetField(tif, EXIFTAG_FOCALLENGTH, &value_cnt, &value);
    memcpy(&tags.focal_length, value, sizeof(tags.focal_length));
    TIFFGetField(tif, EXIFTAG_SHUTTERSPEEDVALUE, &value_cnt, &value);
    memcpy(&tags.shutter_speed, value, sizeof(tags.shutter_speed));
   
    auto strip_size = TIFFStripSize(tif);
    auto num_strip = TIFFNumberOfStrips(tif);
    uint32_t offset = 0;
    frame.buff = std::make_shared<insbuff>(tags.width*tags.height*tags.bitpersample/8);
    frame.w = tags.width;
    frame.h = tags.height;

	for (unsigned i = 0; i < num_strip; i++)
	{
        auto ret = TIFFReadRawStrip(tif, i, frame.buff->data()+offset, strip_size);
        if (ret != strip_size)
        {
            LOGERR("read size:%d != strip size:%d", ret, strip_size);
            return INS_ERR;
        }
        offset += strip_size;
    }
    
    TIFFClose(tif);

    return INS_OK;
}

#if 0
printf("dng version:%d%d%d%d bw version:%d%d%d%d make:%s model:%s uniquemodel:%s software:%s datetime:%s\n",
    tags.dng_version[0],
    tags.dng_version[1],
    tags.dng_version[2],
    tags.dng_version[3],
    tags.dng_bw_version[0],
    tags.dng_bw_version[1],
    tags.dng_bw_version[2],
    tags.dng_bw_version[3],
    tags.make.c_str(), 
    tags.model.c_str(),
    tags.uniquecameramodel.c_str(),
    tags.software.c_str(),
    tags.datetime.c_str());

printf("w:%d h:%d bitpersample:%d sampleperpix:%d compression:%d rowperstrip:%d subfiletype:%d, plannaronfig:%d orientation:%d photometric:%d, samplefmt:%d whitelevel:%d blacklevel:%f\n", 
tags.width, 
tags.height,
tags.bitpersample,
tags.sampleperpixel,
tags.compression,
tags.rowperstrip, 
tags.subfiletype, 
tags.plannarconfig, 
tags.orientation,
tags.photometric,
tags.samplefmt,
tags.whitelevel,
tags.blacklevel);

printf("cfalayout:%d calib:%d cfa dim:%d %d patten:%d %d %d %d color:%d %d %d neutral:%f %f %f\n", 
    tags.cfalayout,
    tags.calibrationilluminant1, 
    tags.cfarepeatpatterndim[0],
    tags.cfarepeatpatterndim[1],
    tags.cfapattern[0],
    tags.cfapattern[1],
    tags.cfapattern[2],
    tags.cfapattern[3],
    tags.cfaplanecolor[0],
    tags.cfaplanecolor[1],
    tags.cfaplanecolor[2], 
    tags.asshotneutral[0],
    tags.asshotneutral[1],
    tags.asshotneutral[2]);

printf("matrix:");
for (int i = 0; i < 9; i++)
{
    printf("%f ", tags.colormatrix1[i]);
}
printf("\n");

printf("contrast:%d saturation:%d sharpness:%d wb:%d meter:%d expose program:%d fnumber:%f iso_speed:%f exposure_time:%f exposure_bias_value:%f focal_length:%f shutter_speed:%f\n",
    tags.contrast, 
    tags.saturation,
    tags.sharpness,
    tags.wb,
    tags.metering_mode,
    tags.exposure_program,
    tags.fnumber, 
    tags.iso_speed, 
    tags.exposure_time, 
    tags.exposure_bias_value, 
    tags.focal_length,
    tags.shutter_speed);
#endif

