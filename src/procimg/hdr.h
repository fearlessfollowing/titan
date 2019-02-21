#ifndef _HDR_H_
#define _HDR_H_

#include "iostream"
#include "opencv2/opencv.hpp"

namespace hdr
{
	using namespace std;
	using namespace cv;

	class HDR
	{
	private:
		bool stopFlag;

	public:
		HDR()
		{
			stopFlag = false;
		}
		~HDR()
		{

		}

	private:
		bool motionRemoval(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& img3, cv::Mat& tmp2, cv::Mat& tmp3, cv::Mat& dif3);

		void test_fusion(const Mat& _img1, const Mat& _img2, const Mat& _img3, const Mat& _badMask3, Mat& img, bool isPano);

		void fuseMultiImages(const vector<cv::Mat>& images, cv::Mat& hdrImage, bool isPano);

	public:
		// hdr接口
		// img1: 正常曝光的图像
		// img2: 欠曝光的图像
		// img3: 过曝光的图像
		// imgHDR: 输出HDR图像
		// isPano: 指定输入的图像是不是经纬度图像，如果isPano=true，则认为是经纬度图，将对图像扩展后再做hdr
		// 返回: true为运行成功；false为运行失败，输出的imgHDR无效
		bool runHDR(const Mat& img1, const Mat& img2, const Mat& img3, Mat& imgHDR, bool isPano = false);

		// images[0]: correct exposure
		// images[2*i+1]: under exposure 
		// images[2*i+2]: over exposure 
		// imgHDR: output hdr image
		// isPano: 指定输入的图像是不是经纬度图像，如果isPano=true，则认为是经纬度图，将对图像扩展后再做hdr
		// 返回: true为运行成功；false为运行失败，输出的imgHDR无效
		bool runHDR(const vector<cv::Mat>& images, cv::Mat& imgHDR, bool isPano = false);

		void cancel()
		{
			stopFlag = true;
		}
	};
}


#endif
