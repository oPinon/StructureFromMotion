#include "tests.h"

#include <iostream>
#include <opencv2\opencv.hpp>
#include <vector>

#include "SIFT.h"

using namespace std;
using namespace cv;

int SIFTMatchTest(int argc, char* argv[]) {

	if (argc < 3) {
		cerr << "Command line arguments are : " << endl;
		cerr << "<image1> <image2>" << endl;
		return 1;
	}

	Mat im0 = imread(argv[1]);
	if (im0.empty()) { cerr << "no image " << argv[1] << endl; throw 1; }
	Mat im1 = imread(argv[2]);
	if (im1.empty()) { cerr << "no image " << argv[2] << endl; throw 1; }

	SIFT sift;

	vector<KeyPoint> points0;
	vector<KeyPoint> points1;

	Mat desc0;
	Mat desc1;

	sift.detectAndCompute(im0, noArray(), points0, desc0);
	sift.detectAndCompute(im1, noArray(), points1, desc1);

	vector<DMatch> matches;
	for (uint i0 = 0; i0 < points0.size(); i0++) {
		KeyPoint& p0 = points0[i0];
		float bestDist = INFINITY;
		uint bestMatch = 0;
		uchar* values0 = desc0.data + i0 * 128;

		for (uint i1 = 0; i1 < points1.size(); i1++) {
			uint dist = 0;
			uchar* values1 = desc1.data + i1 * 128;
			for (uint j = 0; j < 128; j++) {
				int d = ((int)values0[j]) - values1[j];
				dist += d*d;
			}
			/* spatial distance match (totally cheating)
			dist = (points0[i0].pt.x - points1[i1].pt.x)*
			(points0[i0].pt.x - points1[i1].pt.x) +
			(points0[i0].pt.y - points1[i1].pt.y)*
			(points0[i0].pt.y - points1[i1].pt.y);*/
			if (dist < bestDist) {
				bestDist = (float) dist;
				bestMatch = i1;
			}
		}
		matches.push_back(DMatch(i0, bestMatch, bestDist));
	}

	Mat dst;
	cv::drawMatches(im0, points0, im1, points1, matches, dst);
	cv::imshow("Matching", dst); cv::waitKey();

	return 0;
}