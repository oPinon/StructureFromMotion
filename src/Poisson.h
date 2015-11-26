
#include <opencv2\opencv.hpp>
#include <vector>

struct Feature {

	cv::Point2f position;
	cv::Vec2f normal = { 0, 1 };

};

struct Gradient {
	uint w, h;
	cv::Mat x, y;

	Gradient(cv::Mat gradX, cv::Mat gradY) : x(gradX), y(gradY) {
		if (gradX.size() != gradY.size()) { std::cerr << "Error : different sizes" << std::endl; throw 1; }
		w = gradX.size().width;
		h = gradX.size().height;
	}

	Gradient(uint w, uint h) : w(w), h(h) {
		x = cv::Mat(cv::Size(w, h), CV_32F);
		y = cv::Mat(cv::Size(w, h), CV_32F);
		x.setTo(cv::Scalar(0));
		y.setTo(cv::Scalar(0));
	}

	cv::Mat display() const {
		cv::Mat dst(cv::Size(w, h), CV_32FC3);
		float* pix = (float*) dst.data;
		float* pix0 = (float*)x.data;
		float* pix1 = (float*)y.data;
		for (uint j = 0; j < h; j++) {
			for (uint i = 0; i < w; i++) {
				pix[3 * (j*w + i) + 0] = 0.5+pix0[j*w + i];
				pix[3 * (j*w + i) + 1] = 0.5+pix1[j*w + i];
				pix[3 * (j*w + i) + 2] = 0.5;
			}
		}
		return dst;
	}

	// sum of gradient's derivatives
	cv::Mat getDivergence() const {

		cv::Mat dx, dy;
		cv::Sobel(x, dx, CV_32F, 1, 0);
		cv::Sobel(y, dy, CV_32F, 0, 1);
		return dx + dy;
	}

	// integrates the gradient by solving a Poisson equation
	cv::Mat poissonIntegration() const {

		cv::Mat targetLap = getDivergence();
		cv::Mat dst(targetLap.size(), CV_32F);
		float* pix = (float*) dst.data;

		// solver iterations
		for (uint k = 0; k < 1000; k++) {
			
			cv::Mat blurred;
			cv::GaussianBlur(dst, blurred, cv::Size(7, 7), 3, 3);
			dst = blurred - targetLap;

			// border constraints
			for (uint i = 0; i < w; i++) {
				pix[i] = 1;
				pix[(h - 1)*w + i] = 1;
			}
			for (uint j = 0; j < h; j++) {
				pix[j*w + 0] = 1;
				pix[j*w + (w - 1)] = 1;
			}
		}

		return dst;
	}
};

float min(float a, float b) {
	return a < b ? a : b;
}

float max(float a, float b) {
	return a > b ? a : b;
}

struct Cloud {
	uint w, h;
	std::vector<Feature> features;

	Gradient reconstructGradient() const {

		Gradient dst(w, h);
		float* pixX = (float*) dst.x.data;
		float* pixY = (float*) dst.y.data;

		for (uint f = 0; f < features.size(); f++) {
			const Feature& feat = features[f];
			const cv::Point2f& c = feat.position;
			float radius = INFINITY;
			for (uint f2 = 0; f2 < features.size(); f2++) {
				if (f == f2) { continue; }
				auto diff = features[f2].position - feat.position;
				float dist = diff.dot(diff);
				if (dist < radius) {
					radius = dist;
				}
			}
			radius = sqrt(radius);
			for (uint i = max(0, c.x - radius); i < min(w - 1, c.x + radius); i++) {
				for (uint j = max(0, c.y - radius); j < min(h - 1, c.y + radius); j++) {
					float dist = (c.x - i)*(c.x - i) + (c.y - j)*(c.y - j);
					if (dist < radius) {
						// gaussian
						float factor = (1/radius) * exp(-(dist*dist)/(2*radius*radius));
						pixX[j*w + i] += feat.normal[0] * factor;
						pixY[j*w + i] += feat.normal[1] * factor;
					}
				}
			}
		}

		return dst;
	}
};

cv::Mat showKeyPoints(const std::vector<Feature>& features, const cv::Mat& src) {

	cv::Mat dst;
	if (src.channels() > 1) {
		src.copyTo(dst);
	}
	else {
		cv::cvtColor(src, dst, cv::COLOR_GRAY2RGB);
	}

	for (const auto& feat : features) {

		cv::Point2f end = {
			feat.position.x + 8 * feat.normal[0],
			feat.position.y + 8 * feat.normal[1]
		};
		cv::arrowedLine(dst, feat.position, end, cv::Scalar(0, 0, 255, 0.5));
	}
	return dst;
}

std::vector<Feature> simulateKeyPoints(cv::Mat src, int quality = 10) {

	uint w = src.size().width, h = src.size().height;
	if (src.channels() > 1) {
		cv::cvtColor(src, src, cv::COLOR_RGB2GRAY);
	}

	// HACK : weird behavior with this function's memory management
	std::vector<cv::KeyPoint>* featuresPtr = new std::vector<cv::KeyPoint>();
	cv::FAST(src, *featuresPtr, quality);
	auto& features = *featuresPtr;

	cv::Mat gradX;
	cv::Sobel(src, gradX, CV_32F, 1, 0);
	cv::Mat gradY;
	cv::Sobel(src, gradY, CV_32F, 0, 1);
	float* gradXp = (float*)gradX.data;
	float* gradYp = (float*)gradY.data;

	std::vector<Feature> dst;

	for (uint i = 0; i < features.size(); i++) {
		auto& pos = features[i].pt;

		uint pixel = ((int)pos.y)*w + ((int)pos.x);
		float gradX = gradXp[pixel];
		float gradY = gradYp[pixel];
		float norm = sqrt(gradX*gradX + gradY*gradY);

		if (norm > 100) {
			Feature feat;
			feat.position = pos;
			feat.normal = cv::Vec2f{ gradX, gradY } / norm;
			dst.push_back(feat);
		}
	}
	// HACK
	free( featuresPtr );

	return dst;
}