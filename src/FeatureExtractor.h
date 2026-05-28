#pragma once
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

struct Features {
	float meanLuminance = 0;
	float luminanceVariance = 0;
	float edgeDensity = 0;
	int numKeypoints = 0;
	float texture = 0;
	// video only
	float motionEnergy = 0;
	float videoRhythm = 0;
};

class FeatureExtractor {
public:
	// extrai todas as features de uma imagem
	Features extractFromImage(ofImage & img);

	// extrai motion energy de um frame de video
	float extractMotionEnergy(ofPixels & prevFrame, ofPixels & currFrame);

private:
	float calcMeanLuminance(cv::Mat & gray);
	float calcLuminanceVariance(cv::Mat & gray, float mean);
	float calcEdgeDensity(cv::Mat & gray);
	int calcKeypoints(cv::Mat & gray);
	float calcTexture(cv::Mat & gray);
};
