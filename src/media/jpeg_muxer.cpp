
#include "jpeg_muxer.h"
#include <sstream>
#include "common.h"
#include "inslog.h"
#include "access_msg_center.h"
#include "Arational.h"
#include "camera_info.h"

int32_t jpeg_muxer::create(std::string url, const uint8_t* data, uint32_t size, const jpeg_metadata* metadata)
{
    auto ret = create_jpeg(url, data, size);
    RETURN_IF_NOT_OK(ret);

    if (metadata && !metadata->raw) {
        set_metadata(url, metadata);
        LOGINFO("exif jpeg:%s created", url.c_str());
    } else {
        LOGINFO("jpeg:%s created", url.c_str());
    }

    return INS_OK;
}

#if 0
// int32_t jpeg_muxer::create_jpeg(std::string url, const uint8_t* data, uint32_t size)
// {
//     int fd = ::open(url.c_str(), O_CREAT | O_WRONLY, 0666);
//     if (fd < 0)
//     {
//         LOGERR("file:%s open fail", url.c_str());
//         return INS_ERR_FILE_OPEN;
//     }

//     int ret = ::write(fd, data, size);
//     if (ret < 0)
//     {
//         LOGERR("file:%s write fail:%d", url.c_str(), ret);
//         ret = INS_ERR_FILE_IO;
//     }
//     else
//     {
//         LOGINFO("jpeg:%s created", url.c_str());
//         ret = INS_OK;
//     }

//     fsync(fd);
//     ::close(fd);

//     return ret;
// }
#endif


int32_t jpeg_muxer::create_jpeg(std::string url, const uint8_t* data, uint32_t size)
{
    FILE* fp = fopen(url.c_str(), "wb");
    if (!fp) {
        LOGERR("jpeg file:%s open fail", url.c_str());
        return INS_ERR_FILE_OPEN;
    }

    auto ret = fwrite(data, 1, size, fp);
    fflush(fp);
    fclose(fp);

    if (ret < (int32_t)size) {
        LOGERR("file:%s write error", url.c_str());
        return INS_ERR_FILE_IO;
    } else {
        return INS_OK;
    }
}

int32_t jpeg_muxer::set_metadata(const std::string& url, const jpeg_metadata* metadata)
{    
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(url);
    if (!image.get()) {
        LOGERR("exiv2 open file:%s fail", url.c_str());
        return INS_ERR_FILE_OPEN; 
	}

    Exiv2::ExifData ed;
	ed["Exif.Image.Make"] = INS_MAKE;
    ed["Exif.Image.Model"] = INS_MODEL;
    ed["Exif.Image.ImageWidth"] = (uint16_t)metadata->width;
    ed["Exif.Image.ImageLength"] = (uint16_t)metadata->height;

    std::string sf = camera_info::get_sn() + "_" + camera_info::get_fw_ver() + "_" + camera_info::get_ver() + "_" + camera_info::get_m_ver();
    ed["Exif.Image.Software"] = sf;

    // struct timeval tv;  
	// gettimeofday(&tv, nullptr);
    int64_t sec = metadata->pts/1000000;
	struct tm *tmt = localtime(&sec);
    char buff[20] = {0};
    snprintf(buff, 20, "%04i:%02i:%02i %02i:%02i:%02i",tmt->tm_year + 1900, tmt->tm_mon + 1, tmt->tm_mday,tmt->tm_hour, tmt->tm_min, tmt->tm_sec);
    ed["Exif.Image.DateTime"] = buff;

    std::stringstream ss;
    ss << metadata->pts; //照片拍摄时间，精确到us，用于防抖
    ed["Exif.Photo.UserComment"] = ss.str().c_str();

    if (metadata->b_exif) 
		set_photo_meta(ed, metadata->exif);

    if (metadata->b_gps) 
		set_gps_meta(ed, metadata->gps);

    if (metadata->b_spherical) {
        Exiv2::XmpData xmp;
        set_sperical_meta(xmp, metadata->width, metadata->height, metadata->map_type);
        image->setXmpData(xmp);
    }

    image->setExifData(ed);
    image->writeMetadata();

    return INS_OK;
}


void jpeg_muxer::set_photo_meta(Exiv2::ExifData& ed, const exif_info& exif)
{
    //闪光灯
    ed["Exif.Photo.Flash"] = (uint16_t)exif.flash;

    //光源
    ed["Exif.Photo.LightSource"] = (uint16_t)exif.lightsource;

    //测光模式
    ed["Exif.Photo.MeteringMode"] = (uint16_t)exif.meter;

    //白平衡
    ed["Exif.Photo.WhiteBalance"] = (uint16_t)exif.awb;

    //iso
    ed["Exif.Photo.ISOSpeedRatings"] = (uint16_t)exif.iso;

    //曝光模式
    ed["Exif.Photo.ExposureProgram"] = (uint16_t)exif.exposure_program;

    //对比度
    ed["Exif.Photo.Contrast"] = (uint16_t)exif.contrast;

    //锐度
    ed["Exif.Photo.Sharpness"] = (uint16_t)exif.sharpness;

    //饱和度
    ed["Exif.Photo.Saturation"] = (uint16_t)exif.saturation;

    //亮度
    ed["Exif.Photo.BrightnessValue"] = Exiv2::Rational(exif.brightness_num, exif.brightness_den);

    //光圈
    ed["Exif.Photo.FNumber"] = Exiv2::URational(exif.f_num, exif.f_den);

    //曝光时间
    ed["Exif.Photo.ExposureTime"] = Exiv2::URational(exif.exp_num, exif.exp_den);

    //曝光补偿
    ed["Exif.Photo.ExposureBiasValue"] = Exiv2::Rational(exif.ev_num, exif.ev_den);

    //快门
    //double shutter_speed_double = log((double)exif.shutter_speed_den/exif.shutter_speed_num)/log(2);
    ed["Exif.Photo.ShutterSpeedValue"] = Exiv2::Rational(exif.shutter_speed_num, exif.shutter_speed_den);

    //焦距
    ed["Exif.Photo.FocalLength"] = Exiv2::URational(exif.focal_length_num, exif.focal_length_den);
}

void jpeg_muxer::set_gps_meta(Exiv2::ExifData& ed, const ins_gps_data& gps)
{
    ed["Exif.GPSInfo.GPSVersionID"] = "2 2 0 0";
    ed["Exif.GPSInfo.GPSStatus"] = (gps.fix_type < 2)?"V":"A";
    if (gps.fix_type < 2) return;

    ed["Exif.GPSInfo.GPSMeasureMode"] = (gps.fix_type == 2)?"2":"3";
    
    ed["Exif.GPSInfo.GPSLatitudeRef"] = (gps.latitude > 0)?"N":"S";
    ed["Exif.GPSInfo.GPSLatitude"] = position_to_string(gps.latitude);

    ed["Exif.GPSInfo.GPSLongitudeRef"] = (gps.longitude > 0)?"E":"W";
    ed["Exif.GPSInfo.GPSLongitude"] = position_to_string(gps.longitude);

    ed["Exif.GPSInfo.GPSAltitudeRef"] = (gps.altitude < 0)?"1":"0";
    ed["Exif.GPSInfo.GPSAltitude"] = Exiv2::URational(abs(gps.altitude*1000), 1000);
    
    ed["Exif.GPSInfo.GPSSpeedRef"] = "K";
    ed["Exif.GPSInfo.GPSSpeed"] = Exiv2::URational(gps.speed_accuracy*1000, 1000);
}

std::string jpeg_muxer::position_to_string(double position)
{
    Arational a, b, c;
    double pos = fabs(position);
    a.num = pos;
    a.den = 1;
    b.num = (pos-a.num)*60;
    b.den = 1; 
    c.num = ((pos-a.num)*60-b.num)*60*1000;
    c.den = 1000;
    std::stringstream ss;
    ss << a.num << "/" << a.den << " " << b.num << "/" << b.den << " " << c.num << "/" << c.den;
    return ss.str();
}

void jpeg_muxer::set_sperical_meta(Exiv2::XmpData& xmp, uint32_t width, uint32_t height, uint32_t map_type)
{
    std::string map = "equirectangular";
    if (map_type != INS_MAP_FLAT) map = "cube";

    xmp["Xmp.GPano.UsePanoramaViewer"] = true;
    xmp["Xmp.GPano.ProjectionType"] = map;
    xmp["Xmp.GPano.CaptureSoftware"] = INS_MODEL;
    xmp["Xmp.GPano.StitchingSoftware"] = INS_MODEL;
    xmp["Xmp.GPano.InitialHorizontalFOVDegrees"] = 100;
    xmp["Xmp.GPano.InitialViewVerticalFOVDegrees"] = 95; 
    xmp["Xmp.GPano.FullPanoWidthPixels"] = width;
    xmp["Xmp.GPano.FullPanoHeightPixels"] = height;
    xmp["Xmp.GPano.CroppedAreaImageWidthPixels"] = width;
    xmp["Xmp.GPano.CroppedAreaImageHeightPixels"] = height;
    xmp["Xmp.GPano.CroppedAreaLeftPixels"] = 0;
    xmp["Xmp.GPano.CroppedAreaTopPixels"] = 0;
}

