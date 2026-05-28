#pragma once
#include "FeatureExtractor.h"
#include "GestureDetector.h"
#include "SimilarityEngine.h"
#include "ofMain.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxXmlSettings.h"

struct MediaItem {
	string path;
	string name;
	bool isVideo;
	ofImage image;
	ofVideoPlayer video;
	bool loaded = false;
	ofTexture texture;
	ofVec3f position;
	float width = 400;
	float height = 300;
	Features features;
	// video features
	ofPixels prevVideoFrame;
	bool hasPrevFrame = false;
	float currentMotionEnergy = 0;
	vector<float> motionHistory;
	int rhythmWindowSize = 30;
	int wallIndex = 0;
	float displayScale   = 1.0f; // escala calculada em arrangeGallery
	float sizeVariation  = 1.0f; // variacao visual baseada em complexidade da imagem (0.75 - 1.25)
};

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void mousePressed(int x, int y, int button);
	void mouseMoved(int x, int y);

	// Media
	vector<MediaItem> mediaItems;
	void loadMedia();
	void sortByLuminance();
	void arrangeGallery();
	void groupAndArrange();

	// XML
	void saveXML(MediaItem & item);
	void loadXML(MediaItem & item);

	// Features
	FeatureExtractor featureExtractor;

	// Similaridade
	SimilarityEngine similarityEngine;
	vector<vector<int>> groups;
	bool groupMode = false;

	// Gestos
	GestureDetector gestureDetector;
	int currentFrameIndex = 0;

	// Galeria 2D
	int cols = 3;
	int thumbW = 300;
	int thumbH = 200;
	int padding = 20;
	int fullscreenIndex = -1;
	int hoverIndex = -1;
	bool sortAscending = true;

	// 3D
	ofEasyCam cam;
	void drawFrames();
	void drawFloor();
	void drawWalls();
	ofVec3f getFrameCenter(int i);
	bool mode3D = true;

	// Fog
	bool showFog = true;
	float fogDensity = 0.0005f;

	// Metadata flutuante
	bool showMetadata = true;

	// Camera ao vivo
	ofVideoGrabber camera;
	bool showCamera = false;
	int camWidth = 640;
	int camHeight = 480;

	// Face detection
	ofxCv::ObjectFinder faceFinder;
};
