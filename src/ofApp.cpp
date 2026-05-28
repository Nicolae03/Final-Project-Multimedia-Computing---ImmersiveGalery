#include "ofApp.h"

// XML - SAVE
void ofApp::saveXML(MediaItem & item) {
	ofxXmlSettings xml;
	xml.addTag("metadata");
	xml.pushTag("metadata");
	xml.addValue("name", item.name);
	xml.addValue("path", item.path);
	xml.addValue("isVideo", item.isVideo);
	xml.addValue("meanLuminance", item.features.meanLuminance);
	xml.addValue("luminanceVariance", item.features.luminanceVariance);
	xml.addValue("edgeDensity", item.features.edgeDensity);
	xml.addValue("numKeypoints", item.features.numKeypoints);
	xml.addValue("texture", item.features.texture);
	xml.addValue("motionEnergy", item.features.motionEnergy);
	xml.addValue("videoRhythm", item.features.videoRhythm);
	xml.popTag();
	string xmlPath = "metadata/" + item.name + ".xml";
	xml.save(xmlPath);
	ofLogNotice() << "XML guardado: " << xmlPath;
}

// XML - LOAD
void ofApp::loadXML(MediaItem & item) {
	string xmlPath = "metadata/" + item.name + ".xml";
	ofxXmlSettings xml;
	if (xml.load(xmlPath)) {
		xml.pushTag("metadata");
		item.features.meanLuminance = xml.getValue("meanLuminance", 0.0f);
		item.features.luminanceVariance = xml.getValue("luminanceVariance", 0.0f);
		item.features.edgeDensity = xml.getValue("edgeDensity", 0.0f);
		item.features.numKeypoints = xml.getValue("numKeypoints", 0);
		item.features.texture = xml.getValue("texture", 0.0f);
		item.features.motionEnergy = xml.getValue("motionEnergy", 0.0f);
		item.features.videoRhythm = xml.getValue("videoRhythm", 0.0f);
		xml.popTag();
		ofLogNotice() << "XML carregado: " << item.name
					  << " lum=" << item.features.meanLuminance
					  << " edges=" << item.features.edgeDensity;
	} else {
		if (!item.isVideo) {
			item.features = featureExtractor.extractFromImage(item.image);
		}
		saveXML(item);
	}
}

// ORDENAR
void ofApp::sortByLuminance() {
	if (sortAscending) {
		sort(mediaItems.begin(), mediaItems.end(), [](const MediaItem & a, const MediaItem & b) {
			return a.features.meanLuminance < b.features.meanLuminance;
		});
	} else {
		sort(mediaItems.begin(), mediaItems.end(), [](const MediaItem & a, const MediaItem & b) {
			return a.features.meanLuminance > b.features.meanLuminance;
		});
	}
	sortAscending = !sortAscending;
	arrangeGallery();
	for (auto & m : mediaItems) {
		if (!m.isVideo) m.image.update();
	}
}

// ORGANIZA QUADROS NAS PAREDES
void ofApp::arrangeGallery() {
	float wallDist    = 600.0f;
	float wallY       = 0;
	float maxHalfSpan = 450.0f;
	float roomHalf    = 588.0f; // limite seguro: parede em 600, margem de 12 unidades
	int n = mediaItems.size();
	if (n == 0) return;

	// quantos itens por parede
	vector<int> perWall(4, 0);
	for (int i = 0; i < n; i++) perWall[i % 4]++;

	for (int i = 0; i < n; i++) {
		int wall  = i % 4;
		int pos   = i / 4;
		int count = perWall[wall];

		// escala adaptativa: cada frame ocupa 85% do seu slot
		float slotSize = (count <= 1) ? 800.0f : (2.0f * maxHalfSpan / count);
		mediaItems[i].displayScale = ofClamp(slotSize * 0.85f / 400.0f, 0.05f, 1.0f);

		// variacao de tamanho baseada em complexidade visual (keypoints + textura)
		float kpNorm  = ofClamp(mediaItems[i].features.numKeypoints / 400.0f, 0.0f, 1.0f);
		float texNorm = ofClamp(mediaItems[i].features.texture       / 500.0f, 0.0f, 1.0f);
		float complexity = (kpNorm + texNorm) * 0.5f;
		mediaItems[i].sizeVariation = 0.75f + complexity * 0.5f; // intervalo: [0.75, 1.25]

		// safeSpan usa o tamanho MAXIMO possivel (1.25x) para nunca clipar na parede adjacente
		float halfFrameW = (mediaItems[i].width * mediaItems[i].displayScale * 1.25f / 2.0f) + 24.0f;
		float safeSpan   = ofClamp(roomHalf - halfFrameW, 0.0f, maxHalfSpan);

		// distribui uniformemente dentro do span seguro
		float offset = (count <= 1) ? 0.0f
			: -safeSpan + pos * (2.0f * safeSpan / (count - 1));

		// edge density como perturbaçao minima (clamped ao span seguro)
		float edgeNorm = ofClamp(mediaItems[i].features.edgeDensity * 10.0f, 0, 1);
		offset += (edgeNorm - 0.5f) * 20.0f;
		offset = ofClamp(offset, -safeSpan, safeSpan);

		mediaItems[i].wallIndex = wall;
		switch (wall) {
		case 0: mediaItems[i].position = ofVec3f(offset, wallY, -wallDist); break;
		case 1: mediaItems[i].position = ofVec3f(offset, wallY,  wallDist); break;
		case 2: mediaItems[i].position = ofVec3f(-wallDist, wallY, offset); break;
		case 3: mediaItems[i].position = ofVec3f( wallDist, wallY, offset); break;
		}
	}
}

// AGRUPA POR SIMILARIDADE
void ofApp::groupAndArrange() {
	vector<Features> allFeatures;
	for (auto & item : mediaItems)
		allFeatures.push_back(item.features);
	groups = similarityEngine.groupBySimilarity(allFeatures, 0.3f);

	float wallDist    = 600.0f;
	float wallY       = 0;
	float maxHalfSpan = 450.0f;
	float roomHalf    = 588.0f;

	for (int g = 0; g < (int)groups.size(); g++) {
		int wall  = g % 4;
		int count = (int)groups[g].size();

		float slotSize = (count <= 1) ? 800.0f : (2.0f * maxHalfSpan / count);
		float scale    = ofClamp(slotSize * 0.85f / 400.0f, 0.05f, 1.0f);

		for (int j = 0; j < count; j++) {
			int i = groups[g][j];

			float kpNorm  = ofClamp(mediaItems[i].features.numKeypoints / 400.0f, 0.0f, 1.0f);
			float texNorm = ofClamp(mediaItems[i].features.texture       / 500.0f, 0.0f, 1.0f);
			mediaItems[i].sizeVariation = 0.75f + (kpNorm + texNorm) * 0.25f;

			float halfFrameW = (mediaItems[i].width * scale * 1.25f / 2.0f) + 24.0f;
			float safeSpan   = ofClamp(roomHalf - halfFrameW, 0.0f, maxHalfSpan);

			float offset = (count <= 1) ? 0.0f
				: -safeSpan + j * (2.0f * safeSpan / (count - 1));
			offset = ofClamp(offset, -safeSpan, safeSpan);

			mediaItems[i].wallIndex    = wall;
			mediaItems[i].displayScale = scale;
			switch (wall) {
			case 0: mediaItems[i].position = ofVec3f(offset, wallY, -wallDist); break;
			case 1: mediaItems[i].position = ofVec3f(offset, wallY,  wallDist); break;
			case 2: mediaItems[i].position = ofVec3f(-wallDist, wallY, offset); break;
			case 3: mediaItems[i].position = ofVec3f( wallDist, wallY, offset); break;
			}
		}
	}
}

// LOAD MEDIA
void ofApp::loadMedia() {
	ofDirectory metaDir("metadata");
	if (!metaDir.exists()) metaDir.create();

	ofDirectory dir("");
	dir.allowExt("jpg");
	dir.allowExt("jpeg");
	dir.allowExt("png");
	dir.allowExt("mp4");
	dir.allowExt("mov");
	dir.allowExt("avi");
	dir.listDir();

	for (int i = 0; i < dir.size(); i++) {
		MediaItem item;
		item.path = dir.getPath(i);
		item.name = dir.getFile(i).getBaseName();
		string ext = ofToLower(dir.getFile(i).getExtension());
		item.isVideo = (ext == "mp4" || ext == "mov" || ext == "avi");

		if (item.isVideo) {
			item.video.load(item.path);
			item.video.setLoopState(OF_LOOP_NORMAL);
			item.video.play();
			float ar = item.video.getWidth() / (float)item.video.getHeight();
			if (ar > 0) {
				item.width = 400;
				item.height = 400 / ar;
			}
		} else {
			item.image.load(item.path);
			item.image.setImageType(OF_IMAGE_COLOR);
			if (item.image.getWidth() > 1920 || item.image.getHeight() > 1080) {
				item.image.resize(1920, 1080);
			}
			float ar = item.image.getWidth() / (float)item.image.getHeight();
			if (ar > 0) {
				item.width = 400;
				item.height = 400 / ar;
				if (item.height > 350) {
					item.height = 350;
					item.width = 350 * ar;
				}
			}
		}

		item.loaded = true;
		loadXML(item);
		mediaItems.push_back(item);
	}

	sortByLuminance();
	arrangeGallery();

	for (auto & m : mediaItems) {
		if (!m.isVideo) m.image.update();
	}
}

// SETUP
void ofApp::setup() {
	ofSetWindowTitle("Immersive Gallery 3D");
	ofBackground(14, 12, 24);
	ofSetFrameRate(60);
	ofSetEscapeQuitsApp(false);

	cam.setPosition(0, 0, 400);
	cam.setNearClip(1);
	cam.setFarClip(5000);

	auto devices = camera.listDevices();
	int camoId = 0;
	for (auto & d : devices) {
		ofLogNotice() << "Camera ID " << d.id << ": " << d.deviceName;
		string name = d.deviceName;
		transform(name.begin(), name.end(), name.begin(), ::tolower);
		if (name.find("camo") != string::npos) {
			camoId = d.id;
			ofLogNotice() << "Camo encontrado no ID " << camoId;
		}
	}
	camera.setDeviceID(camoId);
	camera.setDesiredFrameRate(30);
	camera.setup(camWidth, camHeight);
	ofSleepMillis(500);

	faceFinder.setup("haarcascade_frontalface_default.xml");
	faceFinder.setPreset(ofxCv::ObjectFinder::Fast);

	loadMedia();
}

// UPDATE
void ofApp::update() {
	for (auto & item : mediaItems) {
		if (item.isVideo && item.loaded) {
			item.video.update();

			if (item.video.isFrameNew()) {
				ofPixels currFrame = item.video.getPixels();
				if (item.hasPrevFrame) {
					item.currentMotionEnergy = featureExtractor.extractMotionEnergy(item.prevVideoFrame, currFrame);
					item.features.motionEnergy = item.features.motionEnergy * 0.95f + item.currentMotionEnergy * 0.05f;

					item.motionHistory.push_back(item.currentMotionEnergy);
					if ((int)item.motionHistory.size() > item.rhythmWindowSize) {
						item.motionHistory.erase(item.motionHistory.begin());
					}
					if ((int)item.motionHistory.size() == item.rhythmWindowSize) {
						float mean = 0;
						for (float v : item.motionHistory)
							mean += v;
						mean /= item.rhythmWindowSize;
						float variance = 0;
						for (float v : item.motionHistory)
							variance += (v - mean) * (v - mean);
						variance /= item.rhythmWindowSize;
						item.features.videoRhythm = sqrt(variance);
					}
				}
				item.prevVideoFrame = currFrame;
				item.hasPrevFrame = true;
			}
		}
	}

	if (mode3D) {
		ofVec3f camPos = cam.getPosition();
		for (auto & item : mediaItems) {
			if (!item.isVideo) continue;
			float dist = camPos.distance(item.position);
			float vol = ofMap(dist, 200, 1000, 1.0f, 0.0f, true);
			item.video.setVolume(vol);
		}
	}

	camera.update();
	if (showCamera && camera.isFrameNew()) {
		try {
			faceFinder.update(camera);
			gestureDetector.update(camera.getPixels());
			GestureType gesture = gestureDetector.getGesture();

			if (gesture == GESTURE_SWIPE_LEFT) {
				currentFrameIndex = (currentFrameIndex + 1) % mediaItems.size();
			} else if (gesture == GESTURE_SWIPE_RIGHT) {
				currentFrameIndex = (currentFrameIndex - 1 + mediaItems.size()) % mediaItems.size();
			} else if (gesture == GESTURE_STRONG_MOTION) {
				groupMode = !groupMode;
				if (groupMode)
					groupAndArrange();
				else
					arrangeGallery();
			} else if (gesture == GESTURE_FORWARD) {
				if (!mediaItems.empty()) fullscreenIndex = currentFrameIndex;
			}
		} catch (cv::Exception & e) {
			ofLogError() << "Camera error: " << e.what();
		}
	}
}

// DRAW FLOOR
void ofApp::drawFloor() {
	float size = 1200;
	int tileSize = 100;
	int numTiles = (int)(size / tileSize);
	for (int xi = 0; xi < numTiles; xi++) {
		for (int zi = 0; zi < numTiles; zi++) {
			bool dark = (xi + zi) % 2 == 0;
			ofSetColor(dark ? 48 : 38, dark ? 42 : 33, dark ? 65 : 52);
			float x = -size / 2 + xi * tileSize + tileSize / 2.0f;
			float z = -size / 2 + zi * tileSize + tileSize / 2.0f;
			ofDrawBox(x, -300, z, tileSize, 2, tileSize);
		}
	}
}

// DRAW WALLS
void ofApp::drawWalls() {
	float size = 1200;
	float height = 600;
	float half = size / 2;
	float hh = height / 2;

	// paredes
	ofSetColor(72, 67, 95);
	ofFill();
	ofDrawBox(0,    0, -half, size, height, 6);
	ofDrawBox(0,    0,  half, size, height, 6);
	ofDrawBox(-half, 0,  0,   6, height, size);
	ofDrawBox( half, 0,  0,   6, height, size);

	// chao
	ofSetColor(28, 24, 40);
	ofDrawBox(0, -hh, 0, size, 6, size);

	// tecto
	ofSetColor(20, 17, 32);
	ofDrawBox(0,  hh, 0, size, 6, size);

	// rodape (friso quente na base das paredes)
	ofSetColor(95, 78, 48);
	float baseY  = -hh + 5;
	float baseH  = 10;
	float baseT  = 10;
	ofDrawBox(0,    baseY, -half, size, baseH, baseT);
	ofDrawBox(0,    baseY,  half, size, baseH, baseT);
	ofDrawBox(-half, baseY,    0, baseT, baseH, size);
	ofDrawBox( half, baseY,    0, baseT, baseH, size);
}

// DRAW FRAMES
void ofApp::drawFrames() {
	for (int i = 0; i < (int)mediaItems.size(); i++) {
		MediaItem & item = mediaItems[i];
		ofVec3f pos = item.position;

		ofPushMatrix();
		ofTranslate(pos.x, pos.y, pos.z);

		switch (item.wallIndex) {
		case 0:
			ofRotateDeg(0, 0, 1, 0);
			break;
		case 1:
			ofRotateDeg(180, 0, 1, 0);
			break;
		case 2:
			ofRotateDeg(90, 0, 1, 0);
			break;
		case 3:
			ofRotateDeg(-90, 0, 1, 0);
			break;
		}

		ofTranslate(0, 0, 50);

		float hoverMult = (mode3D && i == hoverIndex) ? 1.3f : 1.0f;
		float w = item.width  * item.displayScale * item.sizeVariation * hoverMult;
		float h = item.height * item.displayScale * item.sizeVariation * hoverMult;

		if (item.isVideo && item.currentMotionEnergy > 0) {
			float motionScale = 1.0f + ofClamp(item.currentMotionEnergy / 50.0f, 0, 0.5f);
			w *= motionScale;
			h *= motionScale;
		}

		// frameSize calculado aqui para ser usado no spotlight e na moldura
		float varNorm   = ofClamp(item.features.luminanceVariance / 5000.0f, 0, 1);
		float frameSize = 8 + varNorm * 16;

		// Spotlight: trapezio 2D na parede — calibrado com w e frameSize
		{
			float beamH = 260.0f;
			float topW  = 18.0f;
			float botW  = w + frameSize * 2; // exatamente foto + moldura dos dois lados
			float baseY = h / 2 + frameSize; // começa no topo da moldura
			ofMesh beam;
			beam.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
			beam.addVertex(ofVec3f(-topW / 2, baseY + beamH, 1));
			beam.addVertex(ofVec3f( topW / 2, baseY + beamH, 1));
			beam.addVertex(ofVec3f(-botW / 2, baseY,         1));
			beam.addVertex(ofVec3f( botW / 2, baseY,         1));
			beam.addColor(ofColor(255, 230, 150, 0));
			beam.addColor(ofColor(255, 230, 150, 0));
			beam.addColor(ofColor(255, 230, 150, 80));
			beam.addColor(ofColor(255, 230, 150, 80));
			ofFill();
			beam.draw();
		}

		// Hover glow (antes da moldura)
		if (mode3D && i == hoverIndex) {
			ofSetColor(255, 205, 70, 65);
			ofFill();
			float glowPad = 20;
			ofDrawRectangle(-w / 2 - glowPad, -h / 2 - glowPad, w + glowPad * 2, h + glowPad * 2);
		}

		// Moldura
		if (item.isVideo && item.features.videoRhythm > 0) {
			float rhythmNorm = ofClamp(item.features.videoRhythm / 10.0f, 0, 1);
			ofSetColor(110 + (int)(rhythmNorm * 145), 75 - (int)(rhythmNorm * 25), 35 - (int)(rhythmNorm * 15));
		} else {
			ofSetColor(105, 82, 48); // madeira mais rica
		}
		ofFill();
		ofDrawRectangle(-w / 2 - frameSize, -h / 2 - frameSize, w + frameSize * 2, h + frameSize * 2);

		// Textura
		ofSetColor(255);
		ofTexture & tex = item.isVideo ? item.video.getTexture() : item.image.getTexture();
		if (tex.isAllocated()) {
			tex.bind();
			ofMesh quad;
			quad.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
			quad.addVertex(ofVec3f(-w / 2, -h / 2, 2));
			quad.addVertex(ofVec3f(w / 2, -h / 2, 2));
			quad.addVertex(ofVec3f(-w / 2, h / 2, 2));
			quad.addVertex(ofVec3f(w / 2, h / 2, 2));
			quad.addTexCoord(ofVec2f(0, tex.getHeight()));
			quad.addTexCoord(ofVec2f(tex.getWidth(), tex.getHeight()));
			quad.addTexCoord(ofVec2f(0, 0));
			quad.addTexCoord(ofVec2f(tex.getWidth(), 0));
			quad.draw();
			tex.unbind();
		} else {
			ofSetColor(60);
			ofDrawRectangle(-w / 2, -h / 2, w, h);
		}

		// Highlight frame atual
		if (i == currentFrameIndex) {
			ofSetColor(255, 255, 255, 80);
			ofNoFill();
			ofDrawRectangle(-w / 2 - frameSize - 6, -h / 2 - frameSize - 6,
				w + (frameSize + 6) * 2, h + (frameSize + 6) * 2);
			ofFill();
		}

		// Nome
		ofSetColor(255, 220, 0);
		ofDrawBitmapString(item.name, -w / 2, h / 2 + 25);

		// Barra luminancia
		float lum = item.features.meanLuminance / 255.0f;
		ofSetColor(40);
		ofDrawRectangle(-w / 2, h / 2 + 30, w, 8);
		ofSetColor(255, 220, 0);
		ofDrawRectangle(-w / 2, h / 2 + 30, w * lum, 8);

		// Barra edge density
		float edge = ofClamp(item.features.edgeDensity * 5.0f, 0, 1);
		ofSetColor(40);
		ofDrawRectangle(-w / 2, h / 2 + 42, w, 8);
		ofSetColor(100, 180, 255);
		ofDrawRectangle(-w / 2, h / 2 + 42, w * edge, 8);

		// Barra motion + rhythm
		if (item.isVideo) {
			float motion = ofClamp(item.currentMotionEnergy / 30.0f, 0, 1);
			ofSetColor(40);
			ofDrawRectangle(-w / 2, h / 2 + 54, w, 8);
			ofSetColor(80, 220, 80);
			ofDrawRectangle(-w / 2, h / 2 + 54, w * motion, 8);

			float rhythm = ofClamp(item.features.videoRhythm / 10.0f, 0, 1);
			ofSetColor(40);
			ofDrawRectangle(-w / 2, h / 2 + 66, w, 8);
			ofSetColor(255, 140, 0);
			ofDrawRectangle(-w / 2, h / 2 + 66, w * rhythm, 8);
		}

		// Metadata flutuante
		if (showMetadata) {
			ofSetColor(200, 200, 255, 180);
			ofDrawBitmapString("Lum: " + ofToString(item.features.meanLuminance, 1), -w / 2, -h / 2 - 42);
			ofDrawBitmapString("Var: " + ofToString(item.features.luminanceVariance, 0), -w / 2, -h / 2 - 30);
			ofDrawBitmapString("Edge: " + ofToString(item.features.edgeDensity, 3), -w / 2, -h / 2 - 18);
			ofDrawBitmapString("KP: " + ofToString(item.features.numKeypoints), w / 2 - 80, -h / 2 - 42);
			ofDrawBitmapString("Tex: " + ofToString(item.features.texture, 0), w / 2 - 80, -h / 2 - 30);
			if (item.isVideo) {
				ofSetColor(80, 220, 80, 180);
				ofDrawBitmapString("Motion: " + ofToString(item.currentMotionEnergy, 1), w / 2 - 80, -h / 2 - 18);
				ofSetColor(255, 140, 0, 180);
				ofDrawBitmapString("Rhythm: " + ofToString(item.features.videoRhythm, 1), -w / 2, -h / 2 - 54);
			}
		}

		// Indicador de grupo
		if (groupMode) {
			for (int g = 0; g < (int)groups.size(); g++) {
				for (int j = 0; j < (int)groups[g].size(); j++) {
					if (groups[g][j] == i) {
						ofSetColor(ofColor::fromHsb(g * 40, 200, 255));
						ofNoFill();
						ofDrawRectangle(-w / 2 - frameSize - 4, -h / 2 - frameSize - 4,
							w + (frameSize + 4) * 2, h + (frameSize + 4) * 2);
						ofFill();
					}
				}
			}
		}

		ofPopMatrix();
	}
}

// DRAW
void ofApp::draw() {
	if (fullscreenIndex >= 0 && fullscreenIndex < (int)mediaItems.size()) {
		MediaItem & item = mediaItems[fullscreenIndex];
		ofSetColor(255);
		if (item.isVideo) {
			item.video.draw(0, 0, ofGetWidth(), ofGetHeight());
		} else {
			item.image.draw(0, 0, ofGetWidth(), ofGetHeight());
		}
		ofSetColor(255, 0, 0);
		ofDrawBitmapString("BACKSPACE - voltar | SPACE - play/pause", 20, 30);
		ofSetColor(255, 220, 0);
		ofDrawBitmapString("Luminance:  " + ofToString(item.features.meanLuminance, 1), 20, 55);
		ofDrawBitmapString("Variance:   " + ofToString(item.features.luminanceVariance, 1), 20, 70);
		ofDrawBitmapString("Edges:      " + ofToString(item.features.edgeDensity, 4), 20, 85);
		ofDrawBitmapString("Keypoints:  " + ofToString(item.features.numKeypoints), 20, 100);
		ofDrawBitmapString("Texture:    " + ofToString(item.features.texture, 1), 20, 115);
		if (item.isVideo) {
			ofSetColor(80, 220, 80);
			ofDrawBitmapString("Motion:     " + ofToString(item.currentMotionEnergy, 1), 20, 130);
			ofSetColor(255, 140, 0);
			ofDrawBitmapString("Rhythm:     " + ofToString(item.features.videoRhythm, 1), 20, 145);
		}

		if (!mediaItems.empty()) {
			vector<Features> allF;
			for (auto & m : mediaItems)
				allF.push_back(m.features);
			auto similar = similarityEngine.findSimilar(fullscreenIndex, allF, 3);
			ofSetColor(100, 200, 255);
			ofDrawBitmapString("Similar:", 20, 170);
			for (int s = 0; s < (int)similar.size(); s++) {
				ofDrawBitmapString("  " + mediaItems[similar[s].index].name + " (dist=" + ofToString(similar[s].distance, 3) + ")",
					20, 185 + s * 15);
			}
		}
		return;
	}

	if (mode3D) {
		if (showFog) {
			glEnable(GL_FOG);
			glFogi(GL_FOG_MODE, GL_EXP2);
			// cor do fog = cor das paredes normalizadas, para que objetos distantes
			// se dissolvam nas paredes em vez de ficarem escuros
			GLfloat fogColor[] = { 0.282f, 0.263f, 0.373f, 1.0f };
			glFogfv(GL_FOG_COLOR, fogColor);
			glFogf(GL_FOG_DENSITY, fogDensity);
		}
		cam.begin();
		ofEnableDepthTest();
		glDisable(GL_CULL_FACE);
		drawFloor();
		drawWalls();
		drawFrames();
		ofDisableDepthTest();
		cam.end();
		glDisable(GL_CULL_FACE);
		if (showFog) glDisable(GL_FOG);
	} else {
		for (int i = 0; i < (int)mediaItems.size(); i++) {
			int col = i % cols;
			int row = i / cols;
			int x = padding + col * (thumbW + padding);
			int y = padding + row * (thumbH + padding);

			int drawX = x, drawY = y, drawW = thumbW, drawH = thumbH;
			if (i == hoverIndex) {
				int expand = 20;
				drawX = x - expand / 2;
				drawY = y - expand / 2;
				drawW = thumbW + expand;
				drawH = thumbH + expand;
			}

			ofSetColor(255);
			if (mediaItems[i].isVideo) {
				mediaItems[i].video.draw(drawX, drawY, drawW, drawH);
				ofSetColor(255, 0, 0, 180);
				ofDrawRectangle(drawX + drawW - 30, drawY + 5, 25, 15);
				ofSetColor(255);
				ofDrawBitmapString("VID", drawX + drawW - 29, drawY + 16);
			} else {
				mediaItems[i].image.draw(drawX, drawY, drawW, drawH);
			}

			float lum = mediaItems[i].features.meanLuminance / 255.0f;
			ofSetColor(50);
			ofDrawRectangle(drawX, drawY + drawH + 2, drawW, 5);
			ofSetColor(255, 220, 0);
			ofDrawRectangle(drawX, drawY + drawH + 2, drawW * lum, 5);

			float edge = ofClamp(mediaItems[i].features.edgeDensity * 5.0f, 0, 1);
			ofSetColor(50);
			ofDrawRectangle(drawX, drawY + drawH + 9, drawW, 5);
			ofSetColor(100, 180, 255);
			ofDrawRectangle(drawX, drawY + drawH + 9, drawW * edge, 5);

			if (mediaItems[i].isVideo) {
				float motion = ofClamp(mediaItems[i].currentMotionEnergy / 30.0f, 0, 1);
				ofSetColor(50);
				ofDrawRectangle(drawX, drawY + drawH + 16, drawW, 5);
				ofSetColor(80, 220, 80);
				ofDrawRectangle(drawX, drawY + drawH + 16, drawW * motion, 5);

				float rhythm = ofClamp(mediaItems[i].features.videoRhythm / 10.0f, 0, 1);
				ofSetColor(50);
				ofDrawRectangle(drawX, drawY + drawH + 23, drawW, 5);
				ofSetColor(255, 140, 0);
				ofDrawRectangle(drawX, drawY + drawH + 23, drawW * rhythm, 5);
			}

			if (groupMode) {
				bool found = false;
				for (int g = 0; g < (int)groups.size() && !found; g++) {
					for (int j = 0; j < (int)groups[g].size() && !found; j++) {
						if (groups[g][j] == i) {
							ofSetColor(ofColor::fromHsb(g * 40, 200, 255));
							found = true;
						}
					}
				}
				if (!found) ofSetColor(100);
			} else if (i == currentFrameIndex) {
				ofSetColor(255, 255, 255);
			} else {
				ofSetColor(i == hoverIndex ? ofColor(255, 220, 0) : ofColor(100));
			}
			ofNoFill();
			ofDrawRectangle(drawX, drawY, drawW, drawH);
			ofFill();

			ofSetColor(180);
			ofDrawBitmapString(mediaItems[i].name, drawX + 5, drawY + drawH - 5);
		}
	}

	if (showCamera) {
		int camX = ofGetWidth() - camWidth - 10;
		int camY = ofGetHeight() - camHeight - 30;
		ofSetColor(255);
		camera.draw(camX, camY, camWidth, camHeight);
		ofSetColor(0, 255, 0);
		ofNoFill();
		for (int i = 0; i < faceFinder.size(); i++) {
			ofRectangle face = faceFinder.getObject(i);
			ofDrawRectangle(
				camX + face.x * (camWidth / (float)camera.getWidth()),
				camY + face.y * (camHeight / (float)camera.getHeight()),
				face.width * (camWidth / (float)camera.getWidth()),
				face.height * (camHeight / (float)camera.getHeight()));
		}
		ofFill();
		ofSetColor(0, 255, 0);
		ofDrawBitmapString("CAM | Faces: " + ofToString(faceFinder.size()), camX, camY - 5);
		float energy = gestureDetector.getMotionEnergy();
		ofSetColor(255, 150, 0);
		ofDrawBitmapString("Motion: " + ofToString(energy, 1) + "%", camX, camY - 20);
		GestureType g = gestureDetector.getGesture();
		string gestureStr = "NONE";
		if (g == GESTURE_SWIPE_LEFT) gestureStr = "SWIPE LEFT";
		if (g == GESTURE_SWIPE_RIGHT) gestureStr = "SWIPE RIGHT";
		if (g == GESTURE_STRONG_MOTION) gestureStr = "STRONG MOTION";
		if (g == GESTURE_FORWARD) gestureStr = "FORWARD";
		ofDrawBitmapString("Gesture: " + gestureStr, camX, camY - 35);
	}

	ofSetColor(180);
	string ordem = sortAscending ? "escuro->claro" : "claro->escuro";
	string gmode = groupMode ? "ON" : "OFF";
	string fogStr = showFog ? "ON" : "OFF";
	string metaStr = showMetadata ? "ON" : "OFF";
	ofDrawBitmapString("TAB=2D/3D | L=luminance(" + ordem + ") | G=group(" + gmode + ") | F=fog(" + fogStr + ") | M=meta(" + metaStr + ") | C=cam | BACK=voltar", 10, ofGetHeight() - 15);
	ofDrawBitmapString("Modo: " + string(mode3D ? "3D (drag=rotate, scroll=zoom)" : "2D") + " | Frame: " + ofToString(currentFrameIndex), 10, ofGetHeight() - 30);
}

// KEY
void ofApp::keyPressed(int key) {
	if (key == OF_KEY_BACKSPACE) fullscreenIndex = -1;
	if (key == ' ') {
		if (fullscreenIndex >= 0 && mediaItems[fullscreenIndex].isVideo) {
			auto & v = mediaItems[fullscreenIndex].video;
			v.isPaused() ? v.play() : v.setPaused(true);
		}
	}
	if (key == 'l' || key == 'L') sortByLuminance();
	if (key == 'c' || key == 'C') showCamera = !showCamera;
	if (key == 'f' || key == 'F') showFog = !showFog;
	if (key == 'm' || key == 'M') showMetadata = !showMetadata;
	if (key == OF_KEY_TAB) mode3D = !mode3D;
	if (key == 'g' || key == 'G') {
		groupMode = !groupMode;
		if (groupMode)
			groupAndArrange();
		else
			arrangeGallery();
	}

	if (mode3D && fullscreenIndex < 0 && !mediaItems.empty()) {
		if (key == OF_KEY_RIGHT) {
			currentFrameIndex = (currentFrameIndex + 1) % mediaItems.size();
		}
		if (key == OF_KEY_LEFT) {
			currentFrameIndex = (currentFrameIndex - 1 + mediaItems.size()) % mediaItems.size();
		}
		if (key == OF_KEY_RIGHT || key == OF_KEY_LEFT) {
			ofVec3f target = mediaItems[currentFrameIndex].position;
			int wall = mediaItems[currentFrameIndex].wallIndex;
			ofVec3f camPos;
			switch (wall) {
			case 0: camPos = ofVec3f(target.x, 0, target.z + 600); break;
			case 1: camPos = ofVec3f(target.x, 0, target.z - 600); break;
			case 2: camPos = ofVec3f(target.x + 600, 0, target.z); break;
			case 3: camPos = ofVec3f(target.x - 600, 0, target.z); break;
			}
			cam.setPosition(camPos);
			cam.lookAt(target, ofVec3f(0, 1, 0));
		}
		if (key == OF_KEY_RETURN) {
			fullscreenIndex = currentFrameIndex;
		}
	}
}

// FRAME CENTER 3D
ofVec3f ofApp::getFrameCenter(int i) {
	ofVec3f pos = mediaItems[i].position;
	switch (mediaItems[i].wallIndex) {
	case 0: return ofVec3f(pos.x, pos.y, pos.z + 50);
	case 1: return ofVec3f(pos.x, pos.y, pos.z - 50);
	case 2: return ofVec3f(pos.x + 50, pos.y, pos.z);
	case 3: return ofVec3f(pos.x - 50, pos.y, pos.z);
	}
	return pos;
}

// MOUSE
void ofApp::mouseMoved(int x, int y) {
	hoverIndex = -1;
	if (!mode3D) {
		for (int i = 0; i < (int)mediaItems.size(); i++) {
			int col = i % cols;
			int row = i / cols;
			int tx = padding + col * (thumbW + padding);
			int ty = padding + row * (thumbH + padding);
			if (x >= tx && x <= tx + thumbW && y >= ty && y <= ty + thumbH) {
				hoverIndex = i;
				break;
			}
		}
	} else {
		// Projecao 3D: encontra o quadro mais proximo do cursor
		float minDist = 140.0f;
		for (int i = 0; i < (int)mediaItems.size(); i++) {
			ofVec3f screen = cam.worldToScreen(getFrameCenter(i));
			if (screen.z >= 1.0f) continue; // atras da camera
			float d = ofDist(x, y, screen.x, screen.y);
			if (d < minDist) {
				minDist = d;
				hoverIndex = i;
			}
		}
	}
}

void ofApp::mousePressed(int x, int y, int button) {
	if (fullscreenIndex >= 0) return;
	if (!mode3D) {
		for (int i = 0; i < (int)mediaItems.size(); i++) {
			int col = i % cols;
			int row = i / cols;
			int tx = padding + col * (thumbW + padding);
			int ty = padding + row * (thumbH + padding);
			if (x >= tx && x <= tx + thumbW && y >= ty && y <= ty + thumbH) {
				fullscreenIndex = i;
				break;
			}
		}
	}
}
