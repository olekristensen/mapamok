#include "ofMain.h"
#include "ofxAssimpModelLoader.h"
#include "ofxUI.h"
#include "DraggablePoints.h"
#include "MeshUtils.h"
#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

class ofApp : public ofBaseApp {
public:
	ofxUICanvas* gui;
	ofxUIRadio* renderMode;
	
	bool editToggle = true;
	bool loadButton = false;
	bool saveButton = false;
	float backgroundBrightness = 0;
	bool useShader = false;
    bool calibrationReady = false;
	
    ofxAssimpModelLoader model;
	ofMesh mesh;
	ofMesh cornerMesh, imageMesh;
    ofMesh calibrationMesh;
    ofMesh refMesh;
    ofMesh object;
    
    ofVboMesh objectVbo;
    
	ofEasyCam cam;
	DraggablePoints objectPoints;
    cv::Mat rvec, tvec;
    ofMatrix4x4 modelMatrix;
    ofxCv::Intrinsics intrinsics;
    ofShader shader;
    string modelFileName;
    
    float aov = 0.80;
    
    
    
	void setup() {
		ofSetWindowTitle("mapamok");
		setupGui();
		if(ofFile::doesFileExist("model.dae")) {
			loadModel("model.dae");
		}
		cam.setDistance(1);
		cam.setNearClip(.1);
		cam.setFarClip(10);
	}
	enum {
		RENDER_MODE_WIREFRAME_OCCLUDED = 0,
		RENDER_MODE_WIREFRAME_FULL,
		RENDER_MODE_OUTLINE,
		RENDER_MODE_FACES,
	};
	void setupGui() {
		gui = new ofxUICanvas();
		
		ofColor
		cb(64, 192),
		co(192, 192),
		coh(128, 192),
		cf(240, 255),
		cfh(128, 255),
		cp(96, 192),
		cpo(255, 192);
		gui->setUIColors(cb, co, coh, cf, cfh, cp, cpo);
        
		gui->addSpacer();
		gui->addLabel("Calibration");
		gui->addToggle("Edit", &editToggle);
		gui->addButton("Load", &loadButton);
		gui->addButton("Save", &saveButton);
		
		gui->addSpacer();
		gui->addLabel("Render");
		vector<string> renderModes;
		renderModes.push_back("Occluded wireframe");
		renderModes.push_back("Full wireframe");
		renderModes.push_back("Depth Edges");
		renderModes.push_back("Faces");
		renderMode = gui->addRadio("Render", renderModes, OFX_UI_ORIENTATION_VERTICAL, OFX_UI_FONT_MEDIUM);
		renderMode->activateToggle(renderModes[0]);
		
		gui->addSpacer();
		gui->addMinimalSlider("Background", 0, 255, &backgroundBrightness);
		gui->addToggle("Use shader", &useShader);
		
		gui->autoSizeToFitWidgets();
	}
	int getSelection(ofxUIRadio* radio) {
		vector<ofxUIToggle*> toggles = radio->getToggles();
		for(int i = 0; i < toggles.size(); i++) {
			if(toggles[i]->getValue()) {
				return i;
			}
		}
		return -1;
	}
    
    void update(){

        
    }
    
	void draw() {
		ofBackground(backgroundBrightness);
		ofSetColor(255);
		
        if(calibrationReady)
            drawCalibrated();
        else if(editToggle && !calibrationReady)
            drawSetup();
        else{
            imageMesh = mesh;
            project(imageMesh, cam, ofGetWindowRect());
            imageMesh.setMode(OF_PRIMITIVE_TRIANGLES);
            ofEnableDepthTest();
            imageMesh.draw();
            ofDisableDepthTest();
        }
	}
    
    
    
    void drawSetup(ofRectangle foo = ofGetWindowRect()){
        cam.begin(foo);
        ofSetLineWidth(2);
        int renderModeSelection = getSelection(renderMode);
        if(renderModeSelection == RENDER_MODE_FACES) {
            ofEnableDepthTest();
            objectVbo.drawFaces();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            objectVbo.drawWireframe();
        } else if(renderModeSelection == RENDER_MODE_OUTLINE || renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
            prepareRender(true, true, false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            float lineWidth = ofGetStyle().lineWidth;
            if(renderModeSelection == RENDER_MODE_OUTLINE) {
                glPolygonOffset(-lineWidth, -lineWidth);
            } else if(renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
                glPolygonOffset(+lineWidth, +lineWidth);
            }
            glColorMask(false, false, false, false);
            objectVbo.drawFaces();
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            objectVbo.drawWireframe();
            prepareRender(false, false, false);
        }
        
        ofEnableDepthTest();
		float pointSize = 4;
		glPointSize(pointSize);
		ofSetColor(ofColor::red);
		glEnable(GL_POLYGON_OFFSET_POINT);
		glPolygonOffset(-pointSize, -pointSize);
		cornerMesh.drawVertices();
        ofSetColor(ofColor::magenta);
        calibrationMesh.drawVertices();
		glDisable(GL_POLYGON_OFFSET_POINT);
		ofDisableDepthTest();
        
        cam.end();
    }
    
    void drawCalibrated(){
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        
        intrinsics.loadProjectionMatrix(10, 2000);
        applyMatrix(modelMatrix);
        
        
        if(useShader)shader.begin();
        
        ofSetLineWidth(2);
        int renderModeSelection = getSelection(renderMode);
        if(renderModeSelection == RENDER_MODE_FACES) {
            ofEnableDepthTest();
            objectVbo.drawFaces();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            objectVbo.drawWireframe();
        } else if(renderModeSelection == RENDER_MODE_OUTLINE || renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
            prepareRender(true, true, false);
            glEnable(GL_POLYGON_OFFSET_FILL);
            float lineWidth = ofGetStyle().lineWidth;
            if(renderModeSelection == RENDER_MODE_OUTLINE) {
                glPolygonOffset(-lineWidth, -lineWidth);
            } else if(renderModeSelection == RENDER_MODE_WIREFRAME_OCCLUDED) {
                glPolygonOffset(+lineWidth, +lineWidth);
            }
            glColorMask(false, false, false, false);
            objectVbo.drawFaces();
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            objectVbo.drawWireframe();
            prepareRender(false, false, false);
        }
        
        if(useShader) shader.end();
        
        if(editToggle){
            ofEnableDepthTest();
            float pointSize = 4;
            glPointSize(pointSize);
            ofSetColor(ofColor::red);
            glEnable(GL_POLYGON_OFFSET_POINT);
            glPolygonOffset(-pointSize, -pointSize);
            cornerMesh.drawVertices();
            ofSetColor(ofColor::magenta);
            calibrationMesh.drawVertices();
            glDisable(GL_POLYGON_OFFSET_POINT);
            ofDisableDepthTest();
        }
        
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        
        
        if(editToggle){
            drawSetup(ofRectangle(ofGetWindowWidth()-500, 0, 500, 500));
        }
        
    }
    
    
	void loadModel(string filename) {
        modelFileName = filename;

        
		model.loadModel(modelFileName);
		mesh = collapseModel(model);
		centerAndNormalize(mesh);
		
		cornerMesh = mesh;
		cornerMesh.clearIndices();
		cornerMesh.setMode(OF_PRIMITIVE_POINTS);
		cornerMesh.addIndices(getRankedCorners(mesh));
        
        
        objectVbo = object = mesh;
        project(object, cam, ofGetWindowRect());
        
        refMesh = cornerMesh;
        project(cornerMesh, cam, ofGetWindowRect());
        
        int totalPoints = cornerMesh.getNumVertices();
        int numPoints = totalPoints *0.05;
        for(int i = 0 ; i < totalPoints; i+=totalPoints/numPoints){
            objectPoints.add(refMesh.getVertices()[i], object.getVertices()[i]);
        }
        
        if(ofIsStringInString(modelFileName, ".dae"))
            ofStringReplace(modelFileName, ".dae", "");
        if(ofIsStringInString(modelFileName, ".ply"))
            ofStringReplace(modelFileName, ".ply", "");
        
        loadCalibration(ofGetWindowRect(), modelFileName);
	}
    
    void loadCalibration(ofRectangle viewport, string modelname){
        Size2i imageSize(viewport.getWidth(), viewport.getHeight());
        float f = imageSize.width * ofDegToRad(aov); // i think this is wrong, but it's optimized out anyway
        Point2f c = Point2f(imageSize) * (1. / 2);
        Mat1d cameraMatrix = (Mat1d(3, 3) <<
                              f, 0, c.x,
                              0, f, c.y,
                              0, 0, 1);
        
        if(ofFile::doesFileExist(modelname+"-rotation-calibration") && ofFile::doesFileExist(modelname+"-translation-calibration")) {
            loadMat(rvec, modelFileName+"-rotation-calibration");
            loadMat(tvec, modelFileName+"-translation-calibration");
            intrinsics.setup(cameraMatrix, imageSize);
            modelMatrix = makeMatrix(rvec, tvec);
            calibrationReady = true;
        } else {
            calibrationReady = false;
        }

    }
    
    void updateCalibration(ofRectangle viewport) {
        
        Size2i imageSize(viewport.getWidth(), viewport.getHeight());
        float f = imageSize.width * ofDegToRad(aov); // i think this is wrong, but it's optimized out anyway
        Point2f c = Point2f(imageSize) * (1. / 2);
        Mat1d cameraMatrix = (Mat1d(3, 3) <<
                              f, 0, c.x,
                              0, f, c.y,
                              0, 0, 1);
        
        vector<Mat> rvecs, tvecs;
        Mat distCoeffs;
        vector<vector<Point3f> > referenceObjectPoints(1);
        vector<vector<Point2f> > referenceImagePoints(1);
        int n = objectPoints.size();
        for(int i = 0; i < n; i++) {
            referenceObjectPoints[0].push_back(toCv(objectPoints.getObjectPosition(i)));
            referenceImagePoints[0].push_back(toCv(objectPoints.getImagePosition(i)));
        }
        const static int minPoints = 6;
        if(referenceObjectPoints[0].size() >= minPoints) {
            calibrateCamera(referenceObjectPoints, referenceImagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, CV_CALIB_USE_INTRINSIC_GUESS);
            rvec = rvecs[0];
            tvec = tvecs[0];
            intrinsics.setup(cameraMatrix, imageSize);
            saveMat(rvec, modelFileName+"-rotation-calibration");
            saveMat(tvec, modelFileName+"-translation-calibration");
            modelMatrix = makeMatrix(rvec, tvec);
            calibrationReady = true;
        } else {
            calibrationReady = false;
        }
    }
    
    
	void dragEvent(ofDragInfo dragInfo) {
		if(dragInfo.files.size() == 1) {
			string filename = dragInfo.files[0];
			loadModel(filename);
		}
	}
    
    void exit(){
        saveMatrix(modelMatrix, modelFileName);
    }
};

int main() {
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp());
}