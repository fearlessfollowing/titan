#include "hdr.h"

namespace hdr
{
	void histSp(const Mat& img1, Mat& img2, int& brightVal)
	{
		int width = img1.cols;
		int height = img1.rows;
		const uchar*const data1 = (const uchar*const)img1.data;
		uchar* data2 = (uchar*)img2.data;
		int h1[256] = { 0 };
		int h2[256] = { 0 };
		int cx = width / 2;
		int cy = height / 2;
		int thresh = cx*cx*0.9;
		int _w = min(800, img1.cols / 4);
		int stepx = width / _w;
		int stepy = height / _w;
		if (stepx < 1)
		{
			stepx = 1;
		}
		if (stepy < 1)
		{
			stepy = 1;
		}

		vector<int> di2, dj2;
		di2.resize(height);
		dj2.resize(width);
		for (int i = 0; i < height; i += stepy)
		{
			di2[i] = (i - cy)*(i - cy);
		}
		for (int j = 0; j < width; j += stepx)
		{
			dj2[j] = (j - cx)*(j - cx);
		}

		for (int i = 0; i < height; i += stepy)
		{
			for (int j = 0; j < width; j += stepx)
			{
				double dd = di2[i] + dj2[j];
				if (dd > thresh)
				{
				//	continue;
				}
				int id = (i*width + j) * 3;
				h1[data1[id]]++;
				h2[data2[id]]++;
				h1[data1[id + 1]]++;
				h2[data2[id + 1]]++;
				h1[data1[id + 2]]++;
				h2[data2[id + 2]]++;
			}
		}

		for (int i = 1; i < 256; ++i)
		{
			h1[i] += h1[i - 1];
			h2[i] += h2[i - 1];
		}
		uchar map[256] = { 0 };
		int j0 = 0;
		for (int i = 0; i < 256; ++i)
		{
			for (int j = j0; j < 256; ++j)
			{
				if (h1[j] >= h2[i])
				{
					map[i] = j;
					j0 = j;
					break;
				}
			}
		}

		int brightVal1 = 255, brightVal2 = 255;
		for (int i = 0; i < 256; ++i)
		{
			if (map[i] >= 240)
			{
				brightVal1 = i;
				break;
			}
		}
		brightVal2 = map[240];
		brightVal = min(brightVal1, brightVal2);

		int size = width*height * 3;
		for (int i = 0; i < size; i++)
		{
			data2[i] = map[data2[i]];
		}
	}

	void motionRemove(const Mat& img1, Mat& img2, bool dark = false, int thresh = 240)
	{
		float weight[512] = { 1.0f };
		float a = 20, b = 50;
		for (int i = 0; i < 512; ++i)
		{
			float dif = fabs(i - 256 + 0.001);
			if (dif < a)
			{
				weight[i] = 1;
			}
			else if (dif < b)
			{
				weight[i] = (dif - b) / (a - b);
			}
			else
			{
				weight[i] = 0;
			}
		}
		int width = img1.cols;
		int height = img1.rows;
		int channels = img1.channels();
		uchar* data1 = (uchar*)img1.data;
		uchar* data2 = (uchar*)img2.data;
		int size = width*height * 3;
		if (dark)
		{
			for (int i = 0; i < size; i += 3)
			{
				uchar v1 = max(data1[i], max(data1[i + 1], data1[i + 2]));
				uchar v2 = max(data2[i], max(data2[i + 1], data2[i + 2]));
				if (v1 > thresh || v2 > thresh)
				{
					data2[i] = data1[i];
					data2[i + 1] = data1[i + 1];
					data2[i + 2] = data1[i + 2];
					continue;
				}
				float w1 = weight[data1[i] - data2[i] + 256];
				float w2 = weight[data1[i + 1] - data2[i + 1] + 256];
				float w3 = weight[data1[i + 2] - data2[i + 2] + 256];
				float w = min(min(w1, w2), w3);
				data2[i] = data1[i] * w + data2[i] * (1 - w);
				data2[i + 1] = data1[i + 1] * w + data2[i + 1] * (1 - w);
				data2[i + 2] = data1[i + 2] * w + data2[i + 2] * (1 - w);
			}
		}
		else
		{
			for (int i = 0; i < size; i += 3)
			{
				float w1 = weight[data1[i] - data2[i] + 256];
				float w2 = weight[data1[i + 1] - data2[i + 1] + 256];
				float w3 = weight[data1[i + 2] - data2[i + 2] + 256];
				float w = min(min(w1, w2), w3);
				data2[i] = data1[i] * w + data2[i] * (1 - w);
				data2[i + 1] = data1[i + 1] * w + data2[i + 1] * (1 - w);
				data2[i + 2] = data1[i + 2] * w + data2[i + 2] * (1 - w);
			}
		}
	}

	void motionRemove(const cv::Mat& mask, const Mat& img1, Mat& img2, bool dark = false, int thresh = 240)
	{
		float weight[512] = { 1.0f };
		for (int i = 0; i < 512; ++i)
		{
			float dif = fabs(i - 256 + 0.001);
			float w = 1;
			float t1 = 30, t2 = 50;
			if (dif < t1)
			{
				w = 1;
			}
			else if (dif > t2)
			{
				w = 0;
			}
			else
			{
				w = -1.0 / (t2-t1) * dif + t2 / (t2-t1);
			}
			weight[i] = w;
			if (weight[i] > 1)
			{
				weight[i] = 1;
			}
		}
		int width = img1.cols;
		int height = img1.rows;
		int channels = img1.channels();
		uchar* data1 = (uchar*)img1.data;
		uchar* data2 = (uchar*)img2.data;
		int size = width*height * 3;
		if (dark)
		{
			for (int i = 0; i < size; i += 3)
			{
				uchar v1 = max(data1[i], max(data1[i + 1], data1[i + 2]));
				uchar v2 = max(data2[i], max(data2[i + 1], data2[i + 2]));
				if (v1 > thresh || v2 > thresh)
				{
					data2[i] = data1[i];
					data2[i + 1] = data1[i + 1];
					data2[i + 2] = data1[i + 2];
					continue;
				}
				float w1 = weight[data1[i] - data2[i] + 256];
				float w2 = weight[data1[i+1] - data2[i+1] + 256];
				float w3 = weight[data1[i+2] - data2[i+2] + 256];
				float w = min(min(w1, w2), w3);
				data2[i] = data1[i] * w + data2[i] * (1 - w);
				data2[i + 1] = data1[i + 1] * w + data2[i + 1] * (1 - w);
				data2[i + 2] = data1[i + 2] * w + data2[i + 2] * (1 - w);
			}
		}
		else
		{
			uchar* maskData = (uchar*)mask.data;
			for (int i = 0; i < size; i += 3)
			{
				if (maskData[i / 3] == 0)
				{
					data2[i] = data1[i];
					data2[i + 1] = data1[i + 1];
					data2[i + 2] = data1[i + 2];
					continue;
				}
				float w1 = weight[data1[i] - data2[i] + 256];
				float w2 = weight[data1[i + 1] - data2[i + 1] + 256];
				float w3 = weight[data1[i + 2] - data2[i + 2] + 256];
				float w = min(min(w1, w2), w3);
				data2[i] = data1[i] * w + data2[i] * (1 - w);
				data2[i + 1] = data1[i + 1] * w + data2[i + 1] * (1 - w);
				data2[i + 2] = data1[i + 2] * w + data2[i + 2] * (1 - w);
			}
		}
	}

	void computeDif(const cv::Mat& img1, const cv::Mat& img2, cv::Mat& difImg)
	{
		difImg = cv::Mat::zeros(img1.size(), CV_8U);
		uchar* difData = (uchar*)difImg.data;
		uchar* data2 = (uchar*)img2.data;
		uchar* data1 = (uchar*)img1.data;
		int size = difImg.rows*difImg.cols;
		for (int i = 0; i < size; ++i)
		{
			int i3 = i * 3;
			int r1 = data1[i3 + 2];
			int g1 = data1[i3 + 1];
			int b1 = data1[i3];
			int r2 = data2[i3 + 2];
			int g2 = data2[i3 + 1];
			int b2 = data2[i3];
			r1 = abs(r1 - r2);
			g1 = abs(g1 - g2);
			b1 = abs(b1 - b2);
			int v = max(r1, max(g1, b1));
			difData[i] = v;
		}

		cv::blur(difImg, difImg, cv::Size(3, 3));
		for (int i = 0; i < size; ++i)
		{
			if (difData[i] < 32)
			{
				difData[i] = 0;
			}
			else
			{
				difData[i] = 255;
			}
		}

		cv::blur(difImg, difImg, cv::Size(21, 21));
		for (int i = 0; i < size; ++i)
		{
			if (difData[i] < 10)
			{
				difData[i] = 0;
			}
			else
			{
				difData[i] = 255;
			}
		}
	}


	/*
	注：goodVal应根据不同的曝光设置不同的值，对于曝光不足的图片，其过曝的像素应该比曝光
	正常和过曝光的图像获得更大的权值，这么做是为保证对于曝光不足的图片，其亮区的权值跟
	完全过曝是连接的。
	*/
	void computeWeightMap(const cv::Mat& image, cv::Mat& wMap, float goodVal = 127.5)
	{
		//	float goodVal = 127.5;

		wMap = cv::Mat::zeros(image.size(), CV_32F);
		int width = image.cols;
		int height = image.rows;
		uchar* imageData = (uchar*)image.data;
		float* wData = (float*)wMap.data;
		int size = width*height;

		float mapExp[256] = { 0 };
		for (int i = 0; i < 256; ++i)
		{
			mapExp[i] = 255 * powf(2.71828, -(i - goodVal)*(i - goodVal) / (2 * 51 * 51));
		}

		for (int i = 0; i < size; ++i)
		{
			int i3 = i * 3;
			uchar r = imageData[i3 + 2];
			uchar g = imageData[i3 + 1];
			uchar b = imageData[i3];
			wData[i] = max(r, max(g, b)) - min(r, min(g, b)) + mapExp[(r + g + b) / 3];
			// wData[i] =             sat                    +         exp;
		}
	}

	void normalizeWeightMap(cv::Mat& wMap1, cv::Mat& wMap2, cv::Mat& wMap3)
	{
		int size = wMap1.rows*wMap1.cols;
		float* data1 = (float*)wMap1.data;
		float* data2 = (float*)wMap2.data;
		float* data3 = (float*)wMap3.data;
		for (int i = 0; i < size; ++i)
		{
			float sum = data1[i] + data2[i] + data3[i];
			data1[i] /= sum;
			data2[i] /= sum;
			data3[i] /= sum;
			/*if (data1[i] >= data2[i] && data1[i] >= data3[i])
			{
			data1[i] = 1;
			data2[i] = 0;
			data3[i] = 0;
			}
			else if (data2[i] >= data1[i] && data2[i] >= data3[i])
			{
			data1[i] = 0;
			data2[i] = 1;
			data3[i] = 0;
			}
			else
			{
			data1[i] = 0;
			data2[i] = 0;
			data3[i] = 1;
			}*/
		}
	}

	// fuseImage = (A-B)*w+fuseImage
	// A: uchar
	// B: uchar
	// W: float
	// fuseImage: float
	void addToFuse(const cv::Mat& A, const cv::Mat& B, const cv::Mat& W, cv::Mat& fuseImage)
	{
		uchar* AData = (uchar*)A.data;
		uchar* BData = (uchar*)B.data;
		float* WData = (float*)W.data;
		float* fuseData = (float*)fuseImage.data;
		int size = A.rows*A.cols;
		int channels = fuseImage.channels();
		if (channels == 3)
		{
			for (int i = 0; i < size; i++)
			{
				int i3 = i * 3;
				float w = WData[i];

				fuseData[i3] += (AData[i3] - BData[i3])*w;
				fuseData[i3 + 1] += (AData[i3 + 1] - BData[i3 + 1])*w;
				fuseData[i3 + 2] += (AData[i3 + 2] - BData[i3 + 2])*w;
			}
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				float w = WData[i];
				fuseData[i] += (AData[i] - BData[i])*w;
			}
		}
	}

	// fuseImage(i,j) += A(i,j)*W(i,j);
	// A: uchar
	// W: float
	// fuseImage: float
	void addToFuse2(const cv::Mat& A, const cv::Mat& W, cv::Mat& fuseImage)
	{
		uchar* AData = (uchar*)A.data;
		float* WData = (float*)W.data;
		float* fuseData = (float*)fuseImage.data;
		int size = A.rows*A.cols;
		int channels = fuseImage.channels();
		if (channels == 3)
		{
			for (int i = 0; i < size; i++)
			{
				int i3 = i * 3;
				float w = WData[i];
				fuseData[i3] += AData[i3] * w;
				fuseData[i3 + 1] += AData[i3 + 1] * w;
				fuseData[i3 + 2] += AData[i3 + 2] * w;
			}
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				float w = WData[i];
				fuseData[i] += AData[i] * w;
			}
		}
	}

	void multiFrequencyAdd(const cv::Mat& image, const cv::Mat& wImage, cv::Mat& fuseImage)
	{
		cv::Mat L1, L2;
		cv::Mat H;
		cv::Mat LW;
		for (int i = 0; i < 100; ++i)
		{
			int wnd = pow(4, i) + 1;
			int wnd2 = pow(4, i + 1) + 1;
			if (i == 0)
			{
				wnd = 1;
			}
			int threshold = min(image.cols, image.rows) * 5;
			if (wnd2 >= threshold)
			{
				cv::blur(wImage, LW, cv::Size(wnd, wnd));
				addToFuse2(L2, LW, fuseImage);
				break;
			}

			if (i == 0)
			{
				cv::blur(image, L1, cv::Size(wnd, wnd));
			}
			else
			{
				memcpy(L1.data, L2.data, sizeof(uchar)*L1.rows*L1.cols*L1.channels());
			}

			cv::blur(image, L2, cv::Size(wnd2, wnd2));

			cv::blur(wImage, LW, cv::Size(wnd, wnd));

			addToFuse(L1, L2, LW, fuseImage);
		}
	}

	float dehaze2(const cv::Mat& image, double k1, double k2)
	{
		int width = image.cols;
		int height = image.rows;
		int channels = image.channels();
		int wcnt = 400;
		int step = width / wcnt;
		step = max(step, 1);
		float* data = (float*)image.data;
		vector<float> vListLow, vListHigh;
		vListLow.resize((height / step + 1)*(width / step + 1) * channels);
		vListHigh.resize((height / step + 1)*(width / step + 1) * channels);
		int lowid = 0;
		int highid = 0;
		for (int i = 0; i < height; i += step)
		{
			for (int j = 0; j < width; j += step)
			{
				int id = (i*width + j) * channels;
				for (int n = 0; n < channels; ++n)
				{
					float v1 = data[id + channels];
					if (v1 < 20)
					{
						vListLow[lowid] = v1;
						++lowid;
					}
					if (v1 > 255)
					{
						vListHigh[highid] = v1;
						++highid;
					}
				}
			}
		}
		int sumcnt = vListLow.size();
		vListLow.resize(lowid);
		vListHigh.resize(highid);
		sort(vListLow.begin(), vListLow.end());
		sort(vListHigh.begin(), vListHigh.end());

		int id1 = sumcnt*k1;
		int id2 = vListHigh.size() - 1 - sumcnt*k2;
		float low = 0;
		float high = 255;
		if (id1 >= 0 && id1 < vListLow.size())
		{
			low = vListLow[id1];
		}
		if (id2 >= 0 && id2 < vListHigh.size())
		{
			high = vListHigh[id2];
		}
		
		int pixCnt = width*height * channels;
		high -= low;
		for (int i = 0; i < pixCnt; ++i)
		{
			data[i] -= low;
			if (data[i] < 0)
			{
				data[i] = 0;
			}
			if (data[i] > 64)
			{
				float v = (data[i] - 64)*191 / (high - 64) + 64;
				data[i] = min(v, 255.0f);
			}
		}
		return (high-64)/191;
	}

	void computeWeight2(const cv::Mat& src, const cv::Mat& dst, cv::Mat& wImg)
	{
		if (wImg.empty() || wImg.rows != src.rows || wImg.cols != src.cols || wImg.type() != CV_32F)
		{
			wImg = cv::Mat::zeros(src.size(), CV_32F);
		}
		float* wData = (float*)wImg.data;
		uchar* srcData = (uchar*)src.data;
		float* dstData = (float*)dst.data;
		int size = src.cols*src.rows;
		for (int i = 0; i < size; ++i)
		{
			int i3 = i * 3;
			float v1 = (srcData[i3] + srcData[i3 + 1] + srcData[i3 + 2]) / 3.0;
			float v2 = (dstData[i3] + dstData[i3 + 1] + dstData[i3 + 2]) / 3.0;

			if (v2 < 1 && v2 >= 0)
			{
				v2 = 1;
			}
			if (v2 < 0 && v2 >= -1)
			{
				v2 = -1;
			}
			float w = v2 / (v1 + 10);
			wData[i] = w;
		}
	}

	void weightedFuseMultiImages(const vector<cv::Mat>& images, const vector<cv::Mat>& wImgs, cv::Mat& fuseImage)
	{
		fuseImage = cv::Mat::zeros(images[0].size(), CV_32FC3);
		int size = fuseImage.rows*fuseImage.cols;
		for (int i = 0; i < size; ++i)
		{
			int i3 = i * 3;
			float* fuseData = (float*)fuseImage.data;
			for (int imgID = 0; imgID < images.size(); ++imgID)
			{
				uchar* imgData = (uchar*)images[imgID].data;
				float* wData = (float*)wImgs[imgID].data;
				fuseData[i3] += wData[i] * imgData[i3];
				fuseData[i3+1] += wData[i] * imgData[i3+1];
				fuseData[i3+2] += wData[i] * imgData[i3+2];
			}
		}
	}

	void testWeightFuse(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& img3,
		const cv::Mat& wImg1, const cv::Mat& wImg2, const cv::Mat& wImg3, cv::Mat& fuseImage)
	{
		if (fuseImage.empty() || fuseImage.rows != img1.rows || fuseImage.cols != img1.cols || fuseImage.type() != CV_32FC3)
		{
			fuseImage = cv::Mat::zeros(img1.size(), CV_32FC3);
		}
		uchar* data1 = (uchar*)img1.data;
		uchar* data2 = (uchar*)img2.data;
		uchar* data3 = (uchar*)img3.data;
		float* wData1 = (float*)wImg1.data;
		float* wData2 = (float*)wImg2.data;
		float* wData3 = (float*)wImg3.data;
		float* fuseData = (float*)fuseImage.data;
		int size = img1.rows*img1.cols;
		for (int i = 0; i < size; ++i)
		{
			int i3 = i * 3;
			float w1 = wData1[i];
			float w2 = wData2[i];
			float w3 = wData3[i];
			int b = data1[i3] * w1 + data2[i3] * w2 + data3[i3] * w3;
			int g = data1[i3 + 1] * w1 + data2[i3 + 1] * w2 + data3[i3 + 1] * w3;
			int r = data1[i3 + 2] * w1 + data2[i3 + 2] * w2 + data3[i3 + 2] * w3;

			fuseData[i3] = b;
			fuseData[i3 + 1] = g;
			fuseData[i3 + 2] = r;
		}
	}

	void guidedFilter(cv::Mat I, cv::Mat p, int r, double eps, cv::Mat& q)
	{
		/*
		% GUIDEDFILTER   O(N) time implementation of guided filter.
		%
		%   - guidance image: I (should be a gray-scale/single channel image)
		%   - filtering input image: p (should be a gray-scale/single channel image)
		%   - local window radius: r
		%   - regularization parameter: eps
		*/

		cv::Mat _I;
		I.convertTo(_I, CV_64FC1, 1.0 / 255);
		I = _I;

		cv::Mat _p;
		p.convertTo(_p, CV_64FC1, 1.0 / 255);
		p = _p;

		//[hei, wid] = size(I);  
		int hei = I.rows;
		int wid = I.cols;

		r = 2 * r + 1;//因为opencv自带的boxFilter（）中的Size,比如9x9,我们说半径为4 

					  //mean_I = boxfilter(I, r) ./ N;  
		cv::Mat mean_I;
		cv::boxFilter(I, mean_I, CV_64FC1, cv::Size(r, r));

		//mean_p = boxfilter(p, r) ./ N;  
		cv::Mat mean_p;
		cv::boxFilter(p, mean_p, CV_64FC1, cv::Size(r, r));

		//mean_Ip = boxfilter(I.*p, r) ./ N;  
		cv::Mat mean_Ip;
		cv::boxFilter(I.mul(p), mean_Ip, CV_64FC1, cv::Size(r, r));

		//cov_Ip = mean_Ip - mean_I .* mean_p; % this is the covariance of (I, p) in each local patch.  
		cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p);

		//mean_II = boxfilter(I.*I, r) ./ N;  
		cv::Mat mean_II;
		cv::boxFilter(I.mul(I), mean_II, CV_64FC1, cv::Size(r, r));

		//var_I = mean_II - mean_I .* mean_I;  
		cv::Mat var_I = mean_II - mean_I.mul(mean_I);

		//a = cov_Ip ./ (var_I + eps); % Eqn. (5) in the paper;     
		cv::Mat a = cov_Ip / (var_I + eps);

		//b = mean_p - a .* mean_I; % Eqn. (6) in the paper;  
		cv::Mat b = mean_p - a.mul(mean_I);

		//mean_a = boxfilter(a, r) ./ N;  
		cv::Mat mean_a;
		cv::boxFilter(a, mean_a, CV_64FC1, cv::Size(r, r));

		//mean_b = boxfilter(b, r) ./ N;  
		cv::Mat mean_b;
		cv::boxFilter(b, mean_b, CV_64FC1, cv::Size(r, r));

		//q = mean_a .* I + mean_b; % Eqn. (8) in the paper;  
		q = mean_a.mul(I) + mean_b;
	}

	void computeMultiWeights(const vector<cv::Mat>& images, const cv::Mat& fuseImg, vector<cv::Mat>& wImgs)
	{
		vector<float> eMap(256, 0);
		float goodVal = 127.5;
		for (int i = 0; i < 256; ++i)
		{
			if (i < goodVal)
			{
				eMap[i] = i / goodVal;
			}
			else
			{
				eMap[i] = 1 - (i - goodVal) / (255 - goodVal);
			}
			eMap[i] += 0.001;
		}

		int size = images[0].rows*images[0].cols;
		vector<float> wList(images.size(), 0);
		float* fuseData = (float*)fuseImg.data;
		for (int i = 0; i < size; ++i)
		{
			for (int imgID = 0; imgID < images.size(); ++imgID)
			{
				uchar* imgData = (uchar*)images[imgID].data;
				wList[imgID] = eMap[imgData[i]];
			}

			float sum = 1e-6;
			for (int imgID = 0; imgID < images.size(); ++imgID)
			{
				uchar* imgData = (uchar*)images[imgID].data;
				sum += imgData[i] * wList[imgID];
			}
			float v = fuseData[i];
			float k = v / sum;
			for (int imgID = 0; imgID < images.size(); ++imgID)
			{
				float* wData = (float*)wImgs[imgID].data;
				wData[i] = wList[imgID] * k;
			}
		}
	}

	/*
	重新计算权值图，图像中信噪比好、曝光好的像素权值大，确保加权出来的图像与输入的融合图像在亮度上保持
	一致。由于每个像素的rgb使用相同的权值，因此可以保证更真实的色彩
	*/
	void computeWeight3(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& img3, const cv::Mat& fuseImg,
		cv::Mat& wImg1, cv::Mat& wImg2, cv::Mat& wImg3)
	{
		int size = img1.rows*img1.cols;
		uchar* data1 = (uchar*)img1.data;
		uchar* data2 = (uchar*)img2.data;
		uchar* data3 = (uchar*)img3.data;
		float* fuseData = (float*)fuseImg.data;
		float* wData1 = (float*)wImg1.data;
		float* wData2 = (float*)wImg2.data;
		float* wData3 = (float*)wImg3.data;

		float eMap1[256];
		float eMap2[256];
		float eMap3[256];
		float goodVal1 = 127.5;
		float goodVal2 = 200;
		float goodVal3 = 55;
		for (int i = 0; i < 256; ++i)
		{
			if (i < goodVal1)
			{
				eMap1[i] = i / goodVal1;
			}
			else
			{
				eMap1[i] = 1 - (i - goodVal1) / (255 - goodVal1);
			}

			if (i < goodVal2)
			{
				eMap2[i] = i / goodVal2;
			}
			else
			{
				eMap2[i] = 1 - (i - goodVal2) / (255 - goodVal2);
			}

			if (i < goodVal3)
			{
				eMap3[i] = i / goodVal3;
			}
			else
			{
				eMap3[i] = 1 - (i - goodVal3) / (255 - goodVal3);
			}

			eMap1[i] += 0.01;
			eMap2[i] += 0.01;
			eMap3[i] += 0.01;
		}

		int channels = fuseImg.channels();
		for (int i = 0; i < size; ++i)
		{
			int i3 = i * channels;
			int v1 = 0, v2 = 0, v3 = 0;
			float v = 0;
			for (int n = 0; n < channels; ++n)
			{
				v1 += data1[i3 + n];
				v2 += data2[i3 + n];
				v3 += data3[i3 + n];
				v += fuseData[i3 + n];
			}
			v1 /= channels;
			v2 /= channels;
			v3 /= channels;
			v /= channels;
			float w1 = eMap1[v1];
			float w2 = eMap2[v2];
			float w3 = eMap3[v3];

			v1 = max(v1, 1);
			v2 = max(v2, 1);
			v3 = max(v3, 1);
			float k = v / (w1*v1 + w2*v2 + w3*v3);
			w1 *= k;
			w2 *= k;
			w3 *= k;

			wData1[i] = w1;
			wData2[i] = w2;
			wData3[i] = w3;
		}
	}

	/*
	badMask标记的是可能运动的区域和噪声大的区域，将该区域的权值设置为0，以降低噪声和运动的影响
	*/
	void refineWeightMap3(cv::Mat& wMap, const cv::Mat& badMask)
	{
		float* wData = (float*)wMap.data;
		uchar* badData = (uchar*)badMask.data;
		int size = wMap.rows*wMap.cols;
		for (int i = 0; i < size; ++i)
		{
			if (badData[i])
			{
				wData[i] = 0;
			}
		}
	}

	void refineByImg1(const cv::Mat& img1, cv::Mat& fuseImage)
	{
		int size = img1.rows*img1.cols*img1.channels();
		uchar* data1 = (uchar*)img1.data;
		float* fuseData = (float*)fuseImage.data;
		float kList[256];
		for (int i = 0; i < 256; ++i)
		{
			kList[i] = 1;
		}
		double b = log(0.3333) / log(120);
		for (int i = 1; i < 120; ++i)
		{
			kList[i] = 3 * powf(i, b);
		}
		for (int i = 0; i < 256; ++i)
		{
			kList[i] += 0.1;
		}
		for (int i = 0; i < size; ++i)
		{
			int v = data1[i];
			v = max(v, 2);
			if (fuseData[i] > kList[v] * v + 10)
			{
				fuseData[i] = kList[v] * v + 10;
			}
		}

		int size1 = img1.rows*img1.cols;
		for (int i = 0; i < size1; ++i)
		{
			int i3 = i * 3;
			float fr = fuseData[i3 + 2];
			float fg = fuseData[i3 + 1];
			float fb = fuseData[i3];
			int r = data1[i3 + 2];
			int g = data1[i3 + 1];
			int b = data1[i3];
			float fDif = fabs(fr - fg) + fabs(fg - fb) + fabs(fb - fr);
			int dif = abs(r - g) + abs(g - b) + abs(b - r);
			float fAvg = (fr + fg + fb) / 3;
			float avg = (r + g + b +0.1) / 3.0;
			fDif /= fAvg;
			dif /= avg;
			if (dif > fDif)
			{
				if (fDif < 1)
				{
					continue;
				}
				float k = dif / fDif;
				if (k > 3)
				{
					k = 3;
				}
				fuseData[i3 + 2] = (fr - fAvg)*k + fAvg;
				fuseData[i3 + 1] = (fg - fAvg)*k + fAvg;
				fuseData[i3] = (fb - fAvg)*k + fAvg;
			}
		}
	}

	void extend(cv::Mat& image, double k = 0.25)
	{
		int w = k*image.cols;
		cv::Mat tmp = cv::Mat::zeros(image.rows, image.cols + w * 2, image.type());
		image.copyTo(tmp(cv::Rect(w, 0, image.cols, image.rows)));
		image(cv::Rect(0, 0, w, image.rows)).copyTo(tmp(cv::Rect(w + image.cols, 0, w, image.rows)));
		image(cv::Rect(image.cols - w, 0, w, image.rows)).copyTo(tmp(cv::Rect(0, 0, w, image.rows)));
		image = tmp.clone();
	}

	void enhanceStruct(cv::Mat& _I, float ss)
	{
		cv::Mat I = _I.clone();
		cv::Mat smoothImage;
		cv::GaussianBlur(I, smoothImage, cv::Size(1, 1), 0, 0);
		cv::Mat hImg = I - smoothImage;
		cv::Mat gImg = smoothImage.clone();
		guidedFilter(gImg, gImg, 21, 0.02, gImg);
		int type = CV_32FC1;
		if (I.channels() == 3)
		{
			type = CV_32FC3;
		}
		gImg.convertTo(gImg, type);
		gImg *= 255;
		cv::Mat structImage = smoothImage - gImg;
		structImage *= ss;
		smoothImage = structImage + gImg;
		I = smoothImage + hImg;
		float* IData = (float*)I.data;
		float* _IData = (float*)_I.data;
		int size = I.rows*I.cols*I.channels();
		for (int i = 0; i < size; ++i)
		{
			float k = (_IData[i] - 210) / 30.0;
			k = max(k, 0.0f);
			k = min(k, 1.0f);
			if (IData[i] < _IData[i])
			{
				_IData[i] = IData[i];
			}
			else
			{
				_IData[i] = _IData[i] * k + IData[i] * (1-k);
			}
		}
	}

	void normalizeWImages(vector<cv::Mat>& wImgs)
	{
		int size = wImgs[0].rows*wImgs[0].cols;
		float k = 0.8;
		for (int i = 0; i < size; ++i)
		{
			float maxw = 0, minw = 1e10;
			for (int imgID = 0; imgID < wImgs.size(); ++imgID)
			{
				float* wData = (float*)wImgs[imgID].data;
				maxw = max(maxw, wData[i]);
				minw = min(minw, wData[i]);
			}
			float thresh = minw + (maxw - minw) * k;
			float sum = 1e-6;
			for (int imgID = 0; imgID < wImgs.size(); ++imgID)
			{
				float* wData = (float*)wImgs[imgID].data;
				if (wData[i] < thresh)
				{
					wData[i] = 0.001;
				}
				sum += wData[i];
			}
			for (int imgID = 0; imgID < wImgs.size(); ++imgID)
			{
				float* wData = (float*)wImgs[imgID].data;
				wData[i]/=sum;
			}
		}
	}

	float fastAvg(const cv::Mat& img)
	{
		int width = img.cols;
		int height = img.rows;
		int channels = img.channels();
		uchar* data = (uchar*)img.data;
		int xstep = width / 100;
		int ystep = height / 100;
		xstep = max(xstep, 1);
		ystep = max(ystep, 1);

		double sum = 0;
		int cnt = 0;
		for (int i = 0; i < height; i+=ystep)
		{
			for (int j = 0; j < width; j += xstep)
			{
				for (int k = 0; k < channels; ++k)
				{
					sum += data[(i*width + j)*channels + k];
					++cnt;
				}
			}
		}
		return sum / cnt;
	}

	// 对单通道float型数据优化顶部
	void optimizeTop(cv::Mat& image)
	{
		float* data = (float*)image.data;
		int height = image.rows;
		int width = image.cols;
		float avg = 0;
		for (int i = 0; i < width; ++i)
		{
			avg += data[i];
		}
		avg /= width;
		for (int j = 0; j < width; ++j)
		{
			float k = avg / (data[j]+1e-3);
			float a = (1 - k) / (height*0.03);
			for (int i = 0; i < height * 0.03; ++i)
			{
				float k2 = a*i + k;
				data[i*width + j] *= k2;
			}
		}
	}

	void optimizePanoLeftRight(cv::Mat& wImage, cv::Rect roi)
	{
		int width = wImage.cols;
		int height = wImage.rows;
		float* wData = (float*)wImage.data;
		int roiWidth = roi.width;
		int roix = roi.x;
		int len = roix;
		for (int i = 0; i < height; ++i)
		{
			for (int j = roix; j < len; ++j)
			{
				int j2 = j + roiWidth;
				double k = double(j - roix) / roix;
				wData[i*width + j] = (1 - k)*wData[i*width + j] + k*wData[i*width + j2];
			}
			for (int j = roiWidth; j <= roiWidth + len; ++j)
			{
				int j1 = j - roiWidth;
				double k = double(j1) / roix;
				wData[i*width + j] = wData[i*width + j] * (1 - k) + wData[i*width + j1] * k;
			}
		}
	}

	void HDR::fuseMultiImages(const vector<cv::Mat>& images, cv::Mat& hdrImage, bool isPano)
	{
		vector<cv::Mat> scaledGrayImages;
		int resizeWidth = images[0].cols / 4;
		resizeWidth = max(resizeWidth, 1000);
		resizeWidth = min(resizeWidth, images[0].cols);
		int resizeHeight = resizeWidth*images[0].rows / images[0].cols;
		
		vector<cv::Mat> wImgList;

		for (int i = 0; i < images.size(); ++i)
		{
			if (stopFlag)
			{
				return;
			}
			cv::Mat scaledColorImage, scaledGrayImage;
			cv::resize(images[i], scaledColorImage, cv::Size(resizeWidth, resizeHeight));
			if (isPano)
			{
				extend(scaledColorImage);
			}
			cv::cvtColor(scaledColorImage, scaledGrayImage, CV_BGR2GRAY);
			scaledGrayImages.push_back(scaledGrayImage);
			cv::Mat wImg;
			float avg = fastAvg(scaledColorImage);
			float goodVal = 255 - avg;
			goodVal = (goodVal + 127.5) / 2;
			computeWeightMap(scaledColorImage, wImg, goodVal);
			/*if (i == 0)
			{
				computeWeightMap(scaledColorImage, wImg, 130);
			}
			else if (i % 2 == 1)
			{
				computeWeightMap(scaledColorImage, wImg, 150);
			}
			else
			{
				computeWeightMap(scaledColorImage, wImg, 120);
			}*/
			wImgList.push_back(wImg);
		}

		if (stopFlag)
		{
			return;
		}

		normalizeWImages(wImgList);
		if (stopFlag)
		{
			return;
		}

		cv::Mat fuseImage = cv::Mat::zeros(scaledGrayImages[0].size(), CV_32FC1);

		for (int i = 0; i < scaledGrayImages.size(); ++i)
		{
			if (stopFlag)
			{
				return;
			}
			multiFrequencyAdd(scaledGrayImages[i], wImgList[i], fuseImage);
		}
		
		if (stopFlag)
		{
			return;
		}
		float ss = dehaze2(fuseImage, 0.001, 0.0001);
		ss = max(ss, 1.0f);
		ss = (ss - 1)*1.2 + 1;
		enhanceStruct(fuseImage, ss);
		if (stopFlag)
		{
			return;
		}

		computeMultiWeights(scaledGrayImages, fuseImage, wImgList);
		if (stopFlag)
		{
			return;
		}

		vector<cv::Mat> srcWImgList;
		srcWImgList.resize(wImgList.size());
		for (int i = 0; i < wImgList.size(); ++i)
		{
			if (stopFlag)
			{
				return;
			}
			if (isPano)
			{
				int w = (scaledGrayImages[0].cols - resizeWidth) / 2;
				cv::Rect roi;
				roi.x = w;
				roi.y = 0;
				roi.width = resizeWidth;
				roi.height = resizeHeight;
				wImgList[i] = wImgList[i](roi).clone();
			}
			cv::resize(wImgList[i], srcWImgList[i], images[i].size());
		}

		if (stopFlag)
		{
			return;
		}
		weightedFuseMultiImages(images, srcWImgList, hdrImage);
		if (stopFlag)
		{
			return;
		}
	//	refineByImg1(images[0], hdrImage);
		if (stopFlag)
		{
			return;
		}
		hdrImage.convertTo(hdrImage, CV_8UC3);
	}

	void HDR::test_fusion(const Mat& _img1, const Mat& _img2, const Mat& _img3, const Mat& _badMask3, Mat& img, bool isPano)
	{
		cv::Mat img1, img2, img3, badMask3;
		int resizeWidth = _img1.cols / 4;
		resizeWidth = max(resizeWidth, 1000);
		resizeWidth = min(resizeWidth, _img1.cols);
		int resizeHeight = resizeWidth*_img1.rows / _img1.cols;
		if (stopFlag)
		{
			return;
		}

		cv::resize(_img1, img1, cv::Size(resizeWidth, resizeHeight));
		cv::resize(_img2, img2, cv::Size(resizeWidth, resizeHeight));
		cv::resize(_img3, img3, cv::Size(resizeWidth, resizeHeight));
		cv::resize(_badMask3, badMask3, cv::Size(resizeWidth, resizeHeight));
		if (stopFlag)
		{
			return;
		}

		cv::Size resizeSize = img1.size();
		if (isPano)
		{
			extend(img1);
			extend(img2);
			extend(img3);
			extend(badMask3);
		}
		if (stopFlag)
		{
			return;
		}

		int width = img1.cols;
		int height = img1.rows;
		cv::Mat wMap1, wMap2, wMap3;
		computeWeightMap(img1, wMap1, 130);
		computeWeightMap(img2, wMap2, 140);
		computeWeightMap(img3, wMap3, 120);

		if (stopFlag)
		{
			return;
		}

		refineWeightMap3(wMap3, badMask3);

		normalizeWeightMap(wMap1, wMap2, wMap3);

		if (stopFlag)
		{
			return;
		}

		cv::Mat gray1, gray2, gray3;
		cv::cvtColor(img1, gray1, CV_BGR2GRAY);
		cv::cvtColor(img2, gray2, CV_BGR2GRAY);
		cv::cvtColor(img3, gray3, CV_BGR2GRAY);

		if (stopFlag)
		{
			return;
		}

		cv::Mat fuseImage = cv::Mat::zeros(img1.size(), CV_32FC1);

		multiFrequencyAdd(gray1, wMap1, fuseImage);
		if (stopFlag)
		{
			return;
		}

		multiFrequencyAdd(gray2, wMap2, fuseImage);
		if (stopFlag)
		{
			return;
		}

		multiFrequencyAdd(gray3, wMap3, fuseImage);

		if (stopFlag)
		{
			return;
		}

		float ss = dehaze2(fuseImage, 0.0001, 0.001);
		if (stopFlag)
		{
			return;
		}

		ss = max(ss, 1.0f);
		ss = (ss - 1)*1.2 + 1;
		enhanceStruct(fuseImage, ss);

		if (isPano)
		{
			optimizeTop(fuseImage);
		}

		computeWeight3(gray1, gray2, gray3, fuseImage, wMap1, wMap2, wMap3);

		if (stopFlag)
		{
			return;
		}

		if (isPano)
		{
			int w = (img1.cols - resizeSize.width) / 2;
			cv::Rect roi;
			roi.x = w;
			roi.y = 0;
			roi.width = resizeSize.width;
			roi.height = resizeSize.height;
			optimizePanoLeftRight(wMap1, roi);
			optimizePanoLeftRight(wMap2, roi);
			optimizePanoLeftRight(wMap3, roi);
			wMap1 = wMap1(roi).clone();
			wMap2 = wMap2(roi).clone();
			wMap3 = wMap3(roi).clone();
		}
		cv::resize(wMap1, wMap1, _img1.size());
		cv::resize(wMap2, wMap2, _img1.size());
		cv::resize(wMap3, wMap3, _img1.size());

		if (stopFlag)
		{
			return;
		}

		testWeightFuse(_img1, _img2, _img3, wMap1, wMap2, wMap3, fuseImage);
		if (stopFlag)
		{
			return;
		}

		refineByImg1(_img1, fuseImage);
		if (stopFlag)
		{
			return;
		}

		fuseImage.convertTo(img, CV_8UC3);
	}

	bool HDR::motionRemoval(const cv::Mat& img1, const cv::Mat& img2, const cv::Mat& img3, cv::Mat& tmp2, cv::Mat& tmp3, cv::Mat& dif3)
	{
		img1.copyTo(tmp2);
		img1.copyTo(tmp3);
		int brightVal2 = 255, brightVal3 = 255;
		histSp(img2, tmp2, brightVal2);

		if (stopFlag)
		{
			return false;
		}

		histSp(img3, tmp3, brightVal3);
		
		if (stopFlag)
		{
			return false;
		}

		computeDif(tmp3, img3, dif3);

		if (stopFlag)
		{
			return false;
		}

		motionRemove(dif3, img2, tmp2, true, brightVal2);
		if (stopFlag)
		{
			return false;
		}

		motionRemove(dif3, img3, tmp3, false, brightVal3);

		if (stopFlag)
		{
			return false;
		}
	}

	bool HDR::runHDR(const Mat& img1, const Mat& img2, const Mat& img3, Mat& imgHDR, bool isPano)
	{
		cv::Mat tmp2, tmp3, dif3;
		motionRemoval(img1, img2, img3, tmp2, tmp3, dif3);

		test_fusion(img1, tmp2, tmp3, dif3, imgHDR, isPano);
		if (stopFlag)
		{
			return false;
		}

		return true;
	}

	bool HDR::runHDR(const vector<cv::Mat>& images, cv::Mat& imgHDR, bool isPano)
	{
		if (stopFlag)
		{
			return false;
		}
		vector<cv::Mat> imageList;
		imageList.resize(images.size());
		for (int i = 0; i < images.size(); ++i)
		{
			imageList[i] = images[i].clone();
		}
		for (int i = 1; i < imageList.size(); i += 2)
		{
			if (stopFlag)
			{
				return false;
			}
			cv::Mat img2 = imageList[i];
			cv::Mat img3 = imageList[i + 1];
			if (stopFlag)
			{
				return false;
			}

			cv::Mat tmp2, tmp3;
			imageList[0].copyTo(tmp2);
			imageList[0].copyTo(tmp3);
			if (stopFlag)
			{
				return false;
			}

			int brightVal2 = 255, brightVal3 = 255;
			histSp(img2, tmp2, brightVal2);
			histSp(img3, tmp3, brightVal3);
			if (stopFlag)
			{
				return false;
			}

			motionRemove(img2, tmp2, true, brightVal2);
			if (stopFlag)
			{
				return false;
			}

			motionRemove(img3, tmp3, false, brightVal3);
			if (stopFlag)
			{
				return false;
			}

			tmp2.copyTo(imageList[i]);
			tmp3.copyTo(imageList[i + 1]);
		}

		fuseMultiImages(imageList, imgHDR, isPano);
		if (stopFlag)
		{
			return false;
		}
		return true;
	}
}
