#pragma once
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"

enum GestureType {
	GESTURE_NONE,
	GESTURE_SWIPE_LEFT,
	GESTURE_SWIPE_RIGHT,
	GESTURE_STRONG_MOTION,
	GESTURE_FORWARD
};

class GestureDetector {
public:
	void update(ofPixels & pixels);
	GestureType getGesture();
	float getMotionEnergy();
	ofVec2f getCenterOfMass();

private:
	GestureType currentGesture = GESTURE_NONE;
	float motionEnergy = 0;
	ofVec2f centerOfMass;

	cv::Mat prevGray;
	cv::Mat currGray;

	// historico do centro de massa para detetar swipe
	vector<ofVec2f> history;
	int historySize = 10;

	// thresholds
	float swipeThreshold = 60.0f; // pixels de movimento horizontal
	float strongMotionThreshold = 25.0f; // energia de movimento forte
	float forwardThreshold = 40.0f; // movimento vertical para frente

	ofVec2f calcCenterOfMass(cv::Mat & diff);
};
