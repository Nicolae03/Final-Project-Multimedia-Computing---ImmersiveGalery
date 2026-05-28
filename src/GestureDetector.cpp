#include "GestureDetector.h"

void GestureDetector::update(ofPixels & pixels) {
	// converte para grayscale
	cv::Mat frame = ofxCv::toCv(pixels);
	cv::cvtColor(frame, currGray, cv::COLOR_RGB2GRAY);

	currentGesture = GESTURE_NONE;

	if (prevGray.empty()) {
		currGray.copyTo(prevGray);
		return;
	}

	// calcula diferenca entre frames
	cv::Mat diff;
	cv::absdiff(prevGray, currGray, diff);
	cv::threshold(diff, diff, 20, 255, cv::THRESH_BINARY);

	// calcula energia de movimento
	motionEnergy = (float)cv::countNonZero(diff) / (float)(diff.rows * diff.cols) * 100.0f;

	// calcula centro de massa do movimento
	centerOfMass = calcCenterOfMass(diff);

	// adiciona ao historico
	history.push_back(centerOfMass);
	if ((int)history.size() > historySize) {
		history.erase(history.begin());
	}

	// deteta gestos
	if ((int)history.size() >= historySize) {
		ofVec2f first = history.front();
		ofVec2f last = history.back();
		float dx = last.x - first.x;
		float dy = last.y - first.y;

		if (motionEnergy > strongMotionThreshold) {
			currentGesture = GESTURE_STRONG_MOTION;
		} else if (abs(dx) > swipeThreshold && abs(dx) > abs(dy)) {
			currentGesture = (dx < 0) ? GESTURE_SWIPE_LEFT : GESTURE_SWIPE_RIGHT;
			history.clear();
		} else if (dy < -forwardThreshold && abs(dy) > abs(dx)) {
			currentGesture = GESTURE_FORWARD;
			history.clear();
		}
	}

	currGray.copyTo(prevGray);
}

GestureType GestureDetector::getGesture() {
	return currentGesture;
}

float GestureDetector::getMotionEnergy() {
	return motionEnergy;
}

ofVec2f GestureDetector::getCenterOfMass() {
	return centerOfMass;
}

ofVec2f GestureDetector::calcCenterOfMass(cv::Mat & diff) {
	cv::Moments m = cv::moments(diff, true);
	if (m.m00 > 0) {
		return ofVec2f(m.m10 / m.m00, m.m01 / m.m00);
	}
	return ofVec2f(diff.cols / 2, diff.rows / 2);
}
