
#include "exif_parser.h"
#include "inslog.h"
#include "common.h"
#include "exiv2/exiv2.hpp"

int32_t exif_parser::parse(std::string filename, jpeg_metadata& metadata)
{
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
    if (!image.get())
	{
        LOGERR("exiv2 open file:%s fail", filename.c_str());
		return INS_ERR_FILE_OPEN;
	}

    image->readMetadata();
	Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty())
    {
        LOGERR("file:%s no exif data", filename.c_str());
        return INS_ERR;
    }

    Exiv2::ExifData::iterator it;
    it = exifData.findKey(Exiv2::ExifKey("Exif.Image.ImageWidth"));
    if (it != exifData.end())
    {
        metadata.width = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Image.ImageLength"));
    if (it != exifData.end())
    {
        metadata.height = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.Flash"));
    if (it != exifData.end())
    {
        metadata.exif.flash = (int32_t)it->toLong();
        metadata.b_exif = true;
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.LightSource"));
    if (it != exifData.end())
    {
        metadata.exif.lightsource = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.MeteringMode"));
    if (it != exifData.end())
    {
        metadata.exif.meter = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.WhiteBalance"));
    if (it != exifData.end())
    {
        metadata.exif.awb = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ISOSpeedRatings"));
    if (it != exifData.end())
    {
        metadata.exif.iso = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureProgram"));
    if (it != exifData.end())
    {
        metadata.exif.exposure_program = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.Contrast"));
    if (it != exifData.end())
    {
        metadata.exif.contrast = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.Sharpness"));
    if (it != exifData.end())
    {
        metadata.exif.sharpness = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.Saturation"));
    if (it != exifData.end())
    {
        metadata.exif.saturation = (int32_t)it->toLong();
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FNumber"));
    if (it != exifData.end())
    {
        metadata.exif.f_num = it->toRational().first;
        metadata.exif.f_den = it->toRational().second;
    }
    
    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureTime"));
    if (it != exifData.end())
    {
        metadata.exif.exp_num = it->toRational().first;
        metadata.exif.exp_den = it->toRational().second;
    }
    
    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.FocalLength"));
    if (it != exifData.end())
    {
        metadata.exif.focal_length_num = it->toRational().first;
        metadata.exif.focal_length_den = it->toRational().second;
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.BrightnessValue"));
    if (it != exifData.end())
    {
        metadata.exif.brightness_num = it->toRational().first;
        metadata.exif.brightness_den = it->toRational().second;
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ExposureBiasValue"));
    if (it != exifData.end())
    {
        metadata.exif.ev_num = it->toRational().first;
        metadata.exif.ev_den = it->toRational().second;
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.Photo.ShutterSpeedValue"));
    if (it != exifData.end())
    {
        metadata.exif.shutter_speed_num = it->toRational().first;
        metadata.exif.shutter_speed_den = it->toRational().second;
    }

    it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSStatus"));
    if (it != exifData.end())
    {
        metadata.b_gps = true;
        if (it->toString() == "V")
        {
            LOGINFO("GPS in progress");
            metadata.gps.fix_type = 0;
        }
        else
        {
            LOGINFO("GPS location");
            metadata.gps.fix_type = 2;
        }
    }

    if (metadata.b_gps && metadata.gps.fix_type >= 2)
    {
        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSMeasureMode"));
        if (it != exifData.end())
        {
            if (it->toString() == "3")
            {
                metadata.gps.fix_type = 3;
            }
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitude"));
        if (it != exifData.end())
        {
            auto a = it->toRational(0);
            auto b = it->toRational(1);
            auto c = it->toRational(2);
            metadata.gps.latitude = (double)a.first/a.second + (double)b.first/b.second/60.0 + (double)c.first/c.second/60.0/60.0;
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLatitudeRef"));
        if (it != exifData.end())
        {
            if (it->toString() == "S")
            {
                metadata.gps.latitude = -metadata.gps.latitude;
            }
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitude"));
        if (it != exifData.end())
        {
            auto a = it->toRational(0);
            auto b = it->toRational(1);
            auto c = it->toRational(2);
            metadata.gps.longitude = (double)a.first/a.second + (double)b.first/b.second/60.0 + (double)c.first/c.second/60.0/60.0;
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSLongitudeRef"));
        if (it != exifData.end())
        {
            if (it->toString() == "W")
            {
                metadata.gps.longitude = -metadata.gps.longitude;
            }
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSAltitude"));
        if (it != exifData.end())
        {
            metadata.gps.altitude = (float)it->toRational().first/it->toRational().second;
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSAltitudeRef"));
        if (it != exifData.end())
        {
            if (it->toString() == "1")
            {
                metadata.gps.altitude = -metadata.gps.altitude;
            }
        }

        it = exifData.findKey(Exiv2::ExifKey("Exif.GPSInfo.GPSSpeed"));
        if (it != exifData.end())
        {
            metadata.gps.speed_accuracy = (float)it->toRational().first/it->toRational().second;
        }
    }

    #if 0
    LOGINFO("b_exif:%d exp:%d/%d fnumber:%d/%d ev:%d/%d iso:%d meter:%d awb:%d lightsource:%d flash:%d shutter speed:%d/%d "
            "focalLenth:%d/%d contrast:%d sharpnees:%d saturation:%d brightness:%d/%d exposure_program:%d", 
        metadata.b_exif,
		metadata.exif.exp_num, 
		metadata.exif.exp_den,
		metadata.exif.f_num,
		metadata.exif.f_den,
		metadata.exif.ev_num,
		metadata.exif.ev_den,
		metadata.exif.iso,
		metadata.exif.meter,
		metadata.exif.awb,
		metadata.exif.lightsource,
		metadata.exif.flash, 
		metadata.exif.shutter_speed_num, 
		metadata.exif.shutter_speed_den, 
		metadata.exif.focal_length_num, 
		metadata.exif.focal_length_den, 
		metadata.exif.contrast, 
		metadata.exif.sharpness,
		metadata.exif.saturation, 
		metadata.exif.brightness_num, 
        metadata.exif.brightness_den,
        metadata.exif.exposure_program);
    
    LOGINFO("b_gps:%d fixtype:%d latitude:%lf longitude:%lf altitude:%f speed_accuracy:%f", 
        metadata.b_gps, 
        metadata.gps.fix_type, 
        metadata.gps.latitude, 
        metadata.gps.longitude, 
        metadata.gps.altitude, 
        metadata.gps.speed_accuracy);

    // LOGINFO("b_gyro:%d pts:%lld, gyro:%lf %lf %lf accel:%lf %lf %lf b_offset:%d accel offset:%lf %lf %lf", 
    //     metadata.b_gyro,
    //     metadata.gyro.pts, 
    //     metadata.gyro.gyro_x, 
    //     metadata.gyro.gyro_y,
    //     metadata.gyro.gyro_z, 
    //     metadata.gyro.accel_x,
    //     metadata.gyro.accel_y,
    //     metadata.gyro.accel_z,
    //     metadata.b_accel_offset,
    //     metadata.accel_offset.x,
    //     metadata.accel_offset.y,
    //     metadata.accel_offset.z);

    #endif
    
    return INS_OK;
}