#include "ofMain.h"

#include "DraggablePoints.h"

class WhitePoints : public SelectablePoints {
public:
	void draw() {
		ofSetColor(ofColor::white);
		SelectablePoints::draw();
	}
};

class GrayPoints : public DraggablePoints {
public:
	void draw() {
		ofSetColor(ofColor::gray);
		DraggablePoints::draw();
	}
};

class ofApp : public ofBaseApp {
public:
	WhitePoints whitePoints;
	GrayPoints grayPoints;
	
	void setup() {
		whitePoints.setClickRadius(32);
		for(int i = 0; i < 12; i++) {
			whitePoints.add(ofVec2f(ofRandomWidth(), ofRandomHeight()));
		}
		whitePoints.enableControlEvents();
		
		grayPoints.setClickRadius(24);
		for(int i = 0; i < 12; i++) {
			grayPoints.add(ofVec2f(ofRandomWidth(), ofRandomHeight()));
		}
		grayPoints.enableControlEvents();
	}
	void draw() {
		ofBackground(0);
        grayPoints.draw();
	}
};

int main() {
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp());
}