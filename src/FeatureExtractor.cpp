#include "FeatureExtractor.h"

// ── MAIN ENTRY POINT ─────────────────────────────
Features FeatureExtractor::extractFromImage(ofImage & img) {
	Features f;

	// converte para cv::Mat grayscale
	cv::Mat bgr = ofxCv::toCv(img.getPixels());
	cv::Mat gray;
	if (bgr.channels() == 3)
		cv::cvtColor(bgr, gray, cv::COLOR_RGB2GRAY);
	else if (bgr.channels() == 4)
		cv::cvtColor(bgr, gray, cv::COLOR_RGBA2GRAY);
	else
		gray = bgr.clone();

	f.meanLuminance = calcMeanLuminance(gray);
	f.luminanceVariance = calcLuminanceVariance(gray, f.meanLuminance);
	f.edgeDensity = calcEdgeDensity(gray);
	f.numKeypoints = calcKeypoints(gray);
	f.texture = calcTexture(gray);

	return f;
}

// ── MEAN LUMINANCE ───────────────────────────────
float FeatureExtractor::calcMeanLuminance(cv::Mat & gray) {
	cv::Scalar mean = cv::mean(gray);
	return (float)mean[0];
}

// ── LUMINANCE VARIANCE ───────────────────────────
float FeatureExtractor::calcLuminanceVariance(cv::Mat & gray, float mean) {
	cv::Mat floatMat;
	gray.convertTo(floatMat, CV_32F);
	cv::Mat diff = floatMat - mean;
	cv::Mat sq;
	cv::multiply(diff, diff, sq);
	return (float)cv::mean(sq)[0];
}

// ── EDGE DENSITY ─────────────────────────────────
float FeatureExtractor::calcEdgeDensity(cv::Mat & gray) {
	cv::Mat edges;
	cv::Canny(gray, edges, 100, 200);
	// percentagem de pixeis que sao borda
	int edgePixels = cv::countNonZero(edges);
	return (float)edgePixels / (float)(edges.rows * edges.cols);
}

// ── KEYPOINTS (ORB) ──────────────────────────────
int FeatureExtractor::calcKeypoints(cv::Mat & gray) {
	auto orb = cv::ORB::create(500);
	vector<cv::KeyPoint> keypoints;
	orb->detect(gray, keypoints);
	return (int)keypoints.size();
}

// ── TEXTURE (LBP variance) ───────────────────────
float FeatureExtractor::calcTexture(cv::Mat & gray) {
	// usa o desvio padrao local como medida de textura
	cv::Mat mean, stddev;
	cv::Mat floatGray;
	gray.convertTo(floatGray, CV_32F);

	cv::blur(floatGray, mean, cv::Size(9, 9));
	cv::Mat diff = floatGray - mean;
	cv::multiply(diff, diff, diff);
	cv::blur(diff, stddev, cv::Size(9, 9));

	cv::Scalar avgTexture = cv::mean(stddev);
	return (float)avgTexture[0];
}

// ── MOTION ENERGY (video) ────────────────────────
float FeatureExtractor::extractMotionEnergy(ofPixels & prevFrame, ofPixels & currFrame) {
	cv::Mat prev = ofxCv::toCv(prevFrame);
	cv::Mat curr = ofxCv::toCv(currFrame);

	cv::Mat prevGray, currGray, diff;
	cv::cvtColor(prev, prevGray, cv::COLOR_RGB2GRAY);
	cv::cvtColor(curr, currGray, cv::COLOR_RGB2GRAY);

	cv::absdiff(prevGray, currGray, diff);
	cv::Scalar energy = cv::mean(diff);
	return (float)energy[0];
}
