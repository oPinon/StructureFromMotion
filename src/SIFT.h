#pragma once

#include <opencv2\opencv.hpp>

using namespace cv;
using namespace std;

struct SIFT : public Feature2D {

	const uint // used for detection
		sizePyramid = 8,
		maxPoints = 256,
		borderSize = 16; // depends on the descriptor size (in pixels)

	// TODO : mask
	void detect(InputArray image,
		std::vector<KeyPoint>& keypoints,
		InputArray mask = noArray()) {

		// converting to gray scale
		Mat grayImage;
		if (image.channels() > 2) { cvtColor(image, grayImage, COLOR_RGB2GRAY); }
		else { image.copyTo(grayImage); }

		Mat grayF; grayImage.convertTo(grayF, CV_32F); // converting to floats

													   // pyramid of blurred images
		vector<Mat> gaussianPyramid;
		gaussianPyramid.push_back(grayF);

		for (uint i = 0; i < sizePyramid; i++) {
			Mat blurred;
			GaussianBlur(gaussianPyramid[i], blurred, Size(31, 31), 1);
			gaussianPyramid.push_back(blurred);
		}

		// difference between 2 layers
		// absolute difference, because we detect both maximums and minimums
		vector<Mat> diffs;
		for (uint i = 0; i < gaussianPyramid.size() - 1; i++) {
			diffs.push_back(abs(gaussianPyramid[i + 1] - gaussianPyramid[i]));
		}

		// laplacian of the differences (to get maximums)
		vector<Mat> diffsLap;
		float lapArray[] = {
			0, -1, 0,
			-1, 4, -1,
			0, -1, 0
		};
		Mat laplacianKernel(3, 3, CV_32FC1, &lapArray);
		for (uint i = 0; i < diffs.size(); i++) {
			Mat lap;
			filter2D(diffs[i], lap, CV_32F, laplacianKernel);
			diffsLap.push_back(lap);
		}

		KeyPoint a;
		uint w = grayImage.size().width;
		uint h = grayImage.size().height;
		vector<KeyPoint> points;

		for (uint i = 1; i < diffs.size() - 1; i++) {

			float* dat0 = (float*)diffsLap[i - 1].data;
			float* dat1 = (float*)diffsLap[i].data;
			float* dat2 = (float*)diffsLap[i + 1].data;

			for (uint y = borderSize + 1; y < h - borderSize - 1; y++) {
				for (uint x = borderSize + 1; x < w - borderSize - 1; x++) {

					float pBefore = dat0[y*w + x];
					float p = dat1[y*w + x];
					float pAfter = dat2[y*w + x];
					if (p > pBefore && p > pAfter) {

						// TODO : eliminate borders, keep corners only
						// TODO : give more informations to the point
						KeyPoint point;
						point.pt.x = (float)x;
						point.pt.y = (float)y;
						point.octave = i;
						point.response = 2 * p - pBefore - pAfter;
						points.push_back(point);
					}
				}
			}
		}

		sort(points.begin(), points.end(), [](KeyPoint a, KeyPoint b)->bool { return a.response > b.response; });
		points = vector<KeyPoint>(points.begin(), points.begin() + min((uint)points.size(), maxPoints));

		keypoints = points;
	}

	/*
		SIFT is an histogram (8 possible values, from 0 to 360)
		of the gradient angle (weighted by the gradient norm)
		in 16 regions of 4x4 pixels each
	*/
	void compute(const Mat& image, // TODO : InputArray
		CV_OUT CV_IN_OUT std::vector<KeyPoint>& keypoints,
		Mat& descriptors) {

		Mat srcF;
		image.convertTo(srcF, CV_32F);
		if (srcF.channels() > 2) { cvtColor(srcF, srcF, COLOR_RGB2GRAY); } // TODO : useless ?

		descriptors = Mat(Size(128, keypoints.size()), CV_8UC1); // each line is a point
		uchar* descPixels = (uchar*)descriptors.data;

		float* pixels = (float*)srcF.data;
		uint w = srcF.size().width;
		uint h = srcF.size().height;

		for (uint line = 0; line < keypoints.size(); line++) {

			Point2f& pt = keypoints[line].pt;
			for (int regYi = 0; regYi < 4; regYi++) {
				uint y0 = (uint)pt.y + 4 * (regYi - 2);

				for (int regXi = 0; regXi < 4; regXi++) {
					uint x0 = (uint)pt.x + 4 * (regXi - 2);

					float values[8]; // histogram
					for (uint i = 0; i < 8; i++) { values[i] = 0; }
					for (uint y = y0; y < y0 + 4; y++) {
						for (uint x = x0; x < x0 + 4; x++) {

							float p = pixels[y*w + x];
							float gradX = pixels[(y + 1)*w + (x + 0)] - p;
							float gradY = pixels[(y + 0)*w + (x + 1)] - p;

							float norm = sqrt(gradX*gradX + gradY*gradY);
							float angle = atan2(gradY, gradX);
							int bin = (int)((angle + 3.1416f) * 8);

							// TODO : weighted by the distance from the keyPoint center
							//float distX = pt.x - x;
							//float distY = pt.y - y;

							values[max(0, min(8 - 1, bin))] += norm;
						}
					}

					// TODO : shift and normalize the histogram to its center ?
					uint maxPos;
					float maxValue = 0;
					for (uint i = 0; i < 8; i++) {
						if (values[i] > maxValue) {
							maxPos = i;
							maxValue = values[i];
						}
					}
					float shiftedValues[8];
					for (uint i = 0; i < 8; i++) {
						shiftedValues[i] = values[(i + maxPos) % 8] / maxValue;
					}

					uchar* descPos = descPixels + (line * 128 + (regYi * 4 + regXi) * 8);
					for (uint i = 0; i < 8; i++) {
						descPos[i] = (uchar)(255 * shiftedValues[i]);
					}
				}
			}
		}
	};

	void detectAndCompute(const Mat& image, InputArray mask,
		std::vector<KeyPoint>& keypoints,
		Mat& descriptors,
		bool useProvidedKeypoints = false) {

		detect(image, keypoints, mask);
		compute(image, keypoints, descriptors);
	}
};