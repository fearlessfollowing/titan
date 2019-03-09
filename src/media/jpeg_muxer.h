#ifndef _JPEG_MUXER_H_
#define _JPEG_MUXER_H_

#include <string>
#include <stdint.h>
#include <memory>
#include "metadata.h"
#include "exiv2/exiv2.hpp"

class jpeg_muxer {
public:
    int32_t 	create(std::string url, const uint8_t* data, uint32_t size, const jpeg_metadata* metadata = nullptr);
private:
    int32_t 	create_jpeg(std::string url, const uint8_t* data, uint32_t size);
    int32_t 	set_metadata(const std::string& url, const jpeg_metadata* metadata);
    void 		set_photo_meta(Exiv2::ExifData& ed, const exif_info& exif);
    void 		set_gps_meta(Exiv2::ExifData& ed, const ins_gps_data& gps);
    std::string position_to_string(double position);
    void 		set_sperical_meta(Exiv2::XmpData& xmp, uint32_t width, uint32_t height, uint32_t map_type);
};

#endif