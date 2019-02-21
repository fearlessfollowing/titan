
#include "spherical_metadata.h"
#include <sstream>

// std::string spherical_metadata::exif_gpano(int width , int height, int map_type)
// {
//     std::string map = "equirectangular";
//     if (map_type != 0) map = "cube";

//     std::stringstream ss;
//     ss <<
//         " <?xpacket begin=\"?\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>"
//         " <x:xmpmeta xmlns:x='adobe:ns:meta/' x:xmptk='Image::ExifTool 9.46'>"
//         "  <rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>"
//         "   <rdf:Description rdf:about='' xmlns:GPano='http://ns.google.com/photos/1.0/panorama/'>"
//         "    <GPano:UsePanoramaViewer>True</GPano:UsePanoramaViewer>"
//         "    <GPano:ProjectionType>" << map << "</GPano:ProjectionType>"
//         "    <GPano:CaptureSoftware>Insta360 Pro</GPano:CaptureSoftware>"
//         "    <GPano:StitchingSoftware>Insta360 Pro</GPano:StitchingSoftware>"
//         "    <GPano:InitialHorizontalFOVDegrees>100</GPano:InitialHorizontalFOVDegrees>"
//         "    <GPano:InitialViewVerticalFOVDegrees>95</GPano:InitialViewVerticalFOVDegrees>"
//         "    <GPano:FullPanoWidthPixels>" << width << "</GPano:FullPanoWidthPixels>"
//         "    <GPano:FullPanoHeightPixels>" << height << "</GPano:FullPanoHeightPixels>"
//         "    <GPano:CroppedAreaImageWidthPixels>" << width << "</GPano:CroppedAreaImageWidthPixels>"
//         "    <GPano:CroppedAreaImageHeightPixels>" << height << "</GPano:CroppedAreaImageHeightPixels>"
//         "    <GPano:CroppedAreaLeftPixels>" << 0 << "</GPano:CroppedAreaLeftPixels>"
//         "    <GPano:CroppedAreaTopPixels>" << 0 << "</GPano:CroppedAreaTopPixels>"
//         "   </rdf:Description>"
//         "  </rdf:RDF>"
//         " </x:xmpmeta>"
//         " <?xpacket end=\"w\"?>";

//     //  " http://ns.adobe.com/xap/1.0/"
//     //     " <?xpacket begin=\"o;?\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>"
//     //          " <?xpacket end=\"w\"?>"  
    
//     // ss <<
//     //     " <x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Image::ExifTool 9.46\">"
//     //     " <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
//     //     " <rdf:Description rdf:about=\"\" xmlns:GPano=\"http://ns.google.com/photos/1.0/panorama/\""
//     //     " GPano:UsePanoramaViewer=\"True\""
//     //     " GPano:ProjectionType=\"" << map << "\""
//     //     " GPano:InitialHorizontalFOVDegrees=\"100\""
//     //     " GPano:InitialViewVerticalFOVDegrees=\"95\""
//     //     " GPano:FullPanoWidthPixels=\"" << width << "\""
//     //     " GPano:FullPanoHeightPixels=\"" << height << "\""
//     //     " GPano:CroppedAreaImageWidthPixels=\"" << width << "\""
//     //     " GPano:CroppedAreaImageHeightPixels=\"" << height << "\""
//     //     " GPano:CroppedAreaLeftPixels=\"" << 0 << "\""
//     //     " GPano:CroppedAreaTopPixels=\"" << 0 << "\""
//     //     " GPano:CaptureSoftware=\"Insta360 Pro\""
//     //     " GPano:StitchingSoftware=\"Insta360 Pro\""
//     //     " />"
//     //     " </rdf:RDF>"
//     //     " </x:xmpmeta>";

//     return ss.str();
// }

std::string spherical_metadata::video_spherical(std::string mode)
{
    std::stringstream ss;
    ss <<
        "<rdf:SphericalVideo xmlns:GSpherical='http://ns.google.com/videos/1.0/spherical/'\n"
        "xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>\n"
        "<GSpherical:Spherical>true</GSpherical:Spherical>\n"
        "<GSpherical:Stitched>true</GSpherical:Stitched>\n"
        "<GSpherical:ProjectionType>equirectangular</GSpherical:ProjectionType>\n"
        "<GSpherical:StereoMode>" << mode << "</GSpherical:StereoMode>\n"
        "<GSpherical:StitchingSoftware>insta360 pro</GSpherical:StitchingSoftware>\n"  
        "</rdf:SphericalVideo>\n";
    
    return ss.str();
}

std::string spherical_metadata::gps_xmp(const ins_gps_data& data)
{
    float latitude;
    std::string latitude_ref;
    if (data.latitude >= 0)
    {
        latitude = data.latitude;
        latitude_ref = "N";
    }
    else
    {
        latitude = -data.latitude;
        latitude_ref = "S";
    }

    float longitude;
    std::string longitude_ref;
    if (data.longitude >= 0)
    {
        longitude = data.longitude;
        longitude_ref = "E";
    }
    else
    {
        longitude = -data.longitude;
        longitude_ref = "W";
    }

    float altitude = data.altitude*10;

    std::stringstream ss;   
    ss <<   "<x:xmpmeta xmlns:x='adobe:ns:meta/' x:xmptk='Image::ExifTool 9.46'>"
            "<rdf:RDF xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'>"
            "<rdf:Description rdf:about='' xmlns:exif='http://ns.adobe.com/exif/1.0/'>"
            "<exif:GPSLatitude>" << (int)latitude << "," << (latitude - (int)latitude)*60 << latitude_ref << "</exif:GPSLatitude>"
            "<exif:GPSLongitude>" << (int)longitude << "," << (longitude - (int)longitude)*60 << longitude_ref << "</exif:GPSLongitude>"
            "<exif:GPSAltitude>" << (int)altitude << "/10" << "</exif:GPSAltitude>"
            "</rdf:Description>"
            "</rdf:RDF>"
            "</x:xmpmeta>";

    return ss.str();
}