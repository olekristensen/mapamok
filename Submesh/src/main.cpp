#include "ofMain.h"
#include "ofxAssimpModelLoader.h"
#include "MeshUtils.h"

class ofApp : public ofBaseApp{
    
public:
    ofMesh mesh;
    vector<ofMesh> shape;
    string modelFileName;
    ofxAssimpModelLoader model;
    ofEasyCam cam;
    void setup(){
        ofBackground(4, 5, 6);
        ofSetFrameRate(60);
        ofSetVerticalSync(true);
        cam.setDistance(1);
		cam.setNearClip(.1);
		cam.setFarClip(10);
    }
    void update(){
        
    }
    void draw(){
        cam.begin();
        for(int i = 0; i < shape.size(); i++){
            ofSetColor(getColor(i, ofGetElapsedTimef()));
            //prepareRender(true, true, false);
            //glEnable(GL_POLYGON_OFFSET_FILL);
            //float lineWidth = ofGetStyle().lineWidth;
            //glPolygonOffset(-lineWidth, -lineWidth);
            //glColorMask(false, false, false, false);
            shape[i].drawFaces();
            //glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            shape[i].drawWireframe();
            prepareRender(false, false, false);
        }
        cam.end();
    }
    void dragEvent(ofDragInfo dragInfo){
        if(dragInfo.files.size() == 1) {
			string filename = dragInfo.files[0];
            loadModel(filename);
		}
    }
    void loadModel(string filename) {
        modelFileName = filename;
		model.loadModel(modelFileName);
		mesh = collapseModel(model);
		centerAndNormalize(mesh);
        shape = expandModel(model);
		
    }
    ofColor getColor(int x, float t){
        return ofColor(ofMap(cos((x*t)*0.0160), -1, 1, 0, 255), ofMap(sin((x*t)*0.0175), -1, 1, 0, 255), ofMap(sin((x*t)*0.01534), -1, 1, 0, 255));
    }
    
    
};

//========================================================================
int main( ){
	ofSetupOpenGL(1280,800,OF_WINDOW);			// <-------- setup the GL context
    
	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());
    
}
