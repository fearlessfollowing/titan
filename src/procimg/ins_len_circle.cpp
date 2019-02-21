
#include "ins_len_circle.h"
#include <opencv2/opencv.hpp>
#include "CircleFinder/CircleFinder.h"
#include "Lens/Lens.h"
#include <vector>
#include "common.h"

using namespace ins;

int32_t ins_len_circle_find(std::string filename, int32_t version, float& x, float& y, float& r)
{
    std::vector<float> cx, cy, radius;
    Lens::LENSTYPE lensType = Lens::HJ5081;
    Lens::LENSVER lensVer = Lens::PRO1;
    if (version >= 3) lensVer = Lens::PRO2;

    cv::Mat img = cv::imread(filename);
    CircleFinder finder;
    finder.addROI(0, 0, 1, 1);
    finder.setRadius(img.cols, img.rows, lensType, lensVer);
    finder.addImage(img);
    finder.getResult(cx, cy, radius);

    x = cx[0];
    y = cy[0];
    r = radius[0];

    return INS_OK;
}