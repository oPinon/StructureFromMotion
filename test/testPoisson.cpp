#include "tests.h"

#include "Poisson.h"

#include <opencv2\opencv.hpp>
#include <vector>

void testPoisson2D(std::string srcImage) {


	// simulating reconstructed features

	cv::Mat src = cv::imread(srcImage);

	uint w = src.size().width, h = src.size().height;
	if (src.channels() > 1) {
		cv::cvtColor(src, src, cv::COLOR_RGB2GRAY);
	}

	auto dst = simulateKeyPoints(src, 1);

	// displaying them

	cv::imshow("image", showKeyPoints(dst, src));

	Cloud cloud{ w, h, dst };

	cv::Mat gradX;
	cv::Sobel(src, gradX, CV_32F, 1, 0);
	cv::Mat gradY;
	cv::Sobel(src, gradY, CV_32F, 0, 1);
	float* gradXp = (float*)gradX.data;
	float* gradYp = (float*)gradY.data;

	Gradient orgGrad(gradX, gradY);

	cv::imshow("original gradient", 0.5 + 0.001*orgGrad.display());

	Gradient grad = cloud.reconstructGradient();
	cv::imshow("reconstructed gradient", grad.display());

	cv::imshow("reconstructed divergence", 0.5 + grad.getDivergence());

	cv::imshow("divergence", 0.5 + 0.0001*orgGrad.getDivergence());

	cv::waitKey(1);

	cv::imshow("integration", 0.5 + grad.poissonIntegration());

	cv::waitKey();
}