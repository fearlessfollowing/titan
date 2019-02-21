
#include "tiff_mux.h"
#include "tiffio.h"
#include "tif_dir.h"
#include "common.h"
#include "inslog.h"

int tiff_mux::mux(std::string filename, const ins_img_frame& frame, const tiff_tags& tags)
{
    auto tif = TIFFOpen(filename.c_str(), "w");
	if (!tif)
	{
		LOGERR("tiff open file:%s fail", filename.c_str());
		return INS_ERR;
	}
	
	//exif
	auto exif_fields = _TIFFGetExifFields();
	if (exif_fields)
	{
		_TIFFMergeFields(tif, exif_fields->fields, exif_fields->count);

		TIFFSetField(tif, EXIFTAG_EXPOSUREPROGRAM, tags.exposure_program);
		TIFFSetField(tif, EXIFTAG_METERINGMODE, tags.metering_mode);
		TIFFSetField(tif, EXIFTAG_WHITEBALANCE, tags.wb);
		TIFFSetField(tif, EXIFTAG_CONTRAST, tags.contrast);
		TIFFSetField(tif, EXIFTAG_SATURATION, tags.saturation);
		TIFFSetField(tif, EXIFTAG_SHARPNESS, tags.sharpness);
		TIFFSetField(tif, EXIFTAG_EXPOSURETIME,tags.exposure_time);
		TIFFSetField(tif, EXIFTAG_FNUMBER, tags.fnumber);
		TIFFSetField(tif, EXIFTAG_ISOSPEEDRATINGS, 1, tags.iso_speed);
		TIFFSetField(tif, EXIFTAG_EXPOSUREBIASVALUE, tags.exposure_bias_value);
		TIFFSetField(tif, EXIFTAG_FOCALLENGTH, tags.focal_length);
		TIFFSetField(tif, EXIFTAG_SHUTTERSPEEDVALUE, tags.shutter_speed);
	}
	else
	{
		LOGERR("_TIFFGetExifFields fail");
	}

	TIFFSetField(tif, TIFFTAG_DNGVERSION, tags.dng_version);
	TIFFSetField(tif, TIFFTAG_DNGBACKWARDVERSION, tags.dng_bw_version);
	TIFFSetField(tif, TIFFTAG_MAKE, tags.make.c_str());
	TIFFSetField(tif, TIFFTAG_MODEL, tags.model.c_str());
	TIFFSetField(tif, TIFFTAG_UNIQUECAMERAMODEL, tags.uniquecameramodel.c_str());
	TIFFSetField(tif, TIFFTAG_SOFTWARE, tags.software.c_str());
    TIFFSetField(tif, TIFFTAG_DATETIME, tags.datetime.c_str());
    
	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, tags.width);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, tags.height);
	TIFFSetField(tif, TIFFTAG_SUBFILETYPE, tags.subfiletype);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, tags.compression);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, tags.bitpersample);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, tags.orientation);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, tags.photometric);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, tags.sampleperpixel);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, tags.plannarconfig);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, tags.samplefmt);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, tags.rowperstrip);
    TIFFSetField(tif, TIFFTAG_WHITELEVEL, 1, &tags.whitelevel);
	TIFFSetField(tif, TIFFTAG_BLACKLEVEL, 1, &tags.blacklevel);

	TIFFSetField(tif, TIFFTAG_CFAREPEATPATTERNDIM, tags.cfarepeatpatterndim);
	TIFFSetField(tif, TIFFTAG_CFAPATTERN, tags.cfapattern);
	TIFFSetField(tif, TIFFTAG_CFALAYOUT, tags.cfalayout);
    TIFFSetField(tif, TIFFTAG_CFAPLANECOLOR, 3, tags.cfaplanecolor);
    
	TIFFSetField(tif, TIFFTAG_COLORMATRIX1, 9, tags.colormatrix1);
	TIFFSetField(tif, TIFFTAG_ASSHOTNEUTRAL, 3, tags.asshotneutral);
	TIFFSetField(tif, TIFFTAG_CALIBRATIONILLUMINANT1, tags.calibrationilluminant1);

    auto strip_size = tags.width*tags.bitpersample/8;
    auto num_strip = tags.height;
    uint32_t offset = 0;

    for (unsigned i = 0; i < num_strip; i++)
	{
        auto ret = TIFFWriteRawStrip(tif, i, frame.buff->data()+offset, strip_size);
        if (ret != strip_size)
        {
            LOGERR("tiff write size:%d != strip size:%d", ret, strip_size);
            return INS_ERR;
        }
        offset += strip_size;
    }

    TIFFClose(tif);
    
    return INS_OK;
}