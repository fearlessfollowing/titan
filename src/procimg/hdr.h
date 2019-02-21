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
		// hdr�ӿ�
		// img1: �����ع��ͼ��
		// img2: Ƿ�ع��ͼ��
		// img3: ���ع��ͼ��
		// imgHDR: ���HDRͼ��
		// isPano: ָ�������ͼ���ǲ��Ǿ�γ��ͼ�����isPano=true������Ϊ�Ǿ�γ��ͼ������ͼ����չ������hdr
		// ����: trueΪ���гɹ���falseΪ����ʧ�ܣ������imgHDR��Ч
		bool runHDR(const Mat& img1, const Mat& img2, const Mat& img3, Mat& imgHDR, bool isPano = false);

		// images[0]: correct exposure
		// images[2*i+1]: under exposure 
		// images[2*i+2]: over exposure 
		// imgHDR: output hdr image
		// isPano: ָ�������ͼ���ǲ��Ǿ�γ��ͼ�����isPano=true������Ϊ�Ǿ�γ��ͼ������ͼ����չ������hdr
		// ����: trueΪ���гɹ���falseΪ����ʧ�ܣ������imgHDR��Ч
		bool runHDR(const vector<cv::Mat>& images, cv::Mat& imgHDR, bool isPano = false);

		void cancel()
		{
			stopFlag = true;
		}
	};
}


#endif
