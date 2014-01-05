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
    bool firstLoad;
	
	ofxAssimpModelLoader model;
	ofMesh mesh;
	ofMesh cornerMesh, imageMesh;
    ofMesh refMesh;
    ofMesh object;
	ofEasyCam cam;
	SelectablePoints objectPoints;
	DraggablePoints referencePoints;
	
	cv::Mat rvec, tvec;
	ofMatrix4x4 modelMatrix;
	ofxCv::Intrinsics intrinsics;
	bool calibrationReady;
    
    float aov = 0.80;
    
	void setup() {
		ofSetWindowTitle("mapamok");
		setupGui();
		if(ofFile::doesFileExist("model.dae")) {
			loadModel("model.dae");
		}
		cam.setDistance(1);
		cam.setNearClip(.001);
		cam.setFarClip(10);
        

        //referencePoints.enableDrawEvent();
        
	}
	enum {
		RENDER_MODE_WIREFRAME_OCCLUDED = 0,
		RENDER_MODE_WIREFRAME_FULL,
		RENDER_MODE_OUTLINE,
		RENDER_MODE_FACES,
	};
	void setupGui() {
		gui = new ofxUICanvas();
		gui->setFont(OF_TTF_SANS);
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
	void draw() {
		ofBackground(backgroundBrightness);
		ofSetColor(255);
		if(calibrationReady) {
            glPushMatrix();
            glMatrixMode(GL_PROJECTION);
            glPushMatrix();
            glMatrixMode(GL_MODELVIEW);
            
            intrinsics.loadProjectionMatrix(10, 2000);
            applyMatrix(modelMatrix);
            
            ofSetLineWidth(2);
            int renderModeSelection = getSelection(renderMode);
            if(renderModeSelection == RENDER_MODE_FACES) {
                ofEnableDepthTest();
                object.drawFaces();
                ofDisableDepthTest();
            } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
                object.drawWireframe();
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
                object.drawFaces();
                glColorMask(true, true, true, true);
                glDisable(GL_POLYGON_OFFSET_FILL);
                object.drawWireframe();
                prepareRender(false, false, false);
            }
            glPopMatrix();
            glMatrixMode(GL_PROJECTION);
            glPopMatrix();
            glMatrixMode(GL_MODELVIEW);
        }
        
        drawSetup();
    }
    
    void drawSetup(){
        
        //if (firstLoad)
        //cam.begin(ofGetWindowRect());
        //else
        cam.begin(ofRectangle(ofGetWidth()-500, 0, 500, 500));
        ofSetLineWidth(2);
        int renderModeSelection = getSelection(renderMode);
        if(renderModeSelection == RENDER_MODE_FACES) {
            ofEnableDepthTest();
            mesh.drawFaces();
            ofDisableDepthTest();
        } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
            mesh.drawWireframe();
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
            mesh.drawFaces();
            glColorMask(true, true, true, true);
            glDisable(GL_POLYGON_OFFSET_FILL);
            mesh.drawWireframe();
            prepareRender(false, false, false);
            
            //            ofEnableDepthTest();
            //            float pointSize = 4;
            //            glPointSize(pointSize);
            //            ofSetColor(ofColor::magenta);
            //            glEnable(GL_POLYGON_OFFSET_POINT);
            //            glPolygonOffset(-pointSize, -pointSize);
            //            glDisable(GL_POLYGON_OFFSET_POINT);
            //            ofDisableDepthTest();
        }
        
        cam.end();
        if(firstLoad){
            firstLoad = false;
            imageMesh = refMesh;
            project(imageMesh, cam, ofGetWindowRect());
            object = mesh;
            project(object, cam, ofGetWindowRect());
            
            for(int i = 0; i < refMesh.getVertices().size(); i++){
                referencePoints.add(imageMesh.getVertices()[i], imageMesh.getVertices()[i]);
            }
            referencePoints.setClickRadius(6);
            referencePoints.enableControlEvents();
            updateCalibration(ofGetWindowRect());
        }
        
        referencePoints.draw();
    }
    
    void loadModel(string filename) {
        model.loadModel(filename);
        mesh = collapseModel(model);
        centerAndNormalize(mesh);
        
        cornerMesh = mesh;
        cornerMesh.clearIndices();
        cornerMesh.setMode(OF_PRIMITIVE_POINTS);
        cornerMesh.addIndices(getCornerVertices(mesh));
        
        
        vector<ofVec3f> verts = cornerMesh.getVertices();
        int n = verts.size();
        int skip = n/6;
        if(n > 1000)
            for(int i = 0; i < n; i+=skip){
                refMesh.addVertex(verts[i]);
            }
        else
            refMesh.append(cornerMesh);
        
        
        firstLoad = true;
        
    }
    void dragEvent(ofDragInfo dragInfo) {
        if(dragInfo.files.size() == 1) {
            string filename = dragInfo.files[0];
            loadModel(filename);
        }
    }
    
    void keyPressed(int key){
        if(key == OF_KEY_CONTROL)
            cam.disableMouseInput();
        if(key == OF_KEY_ALT)
            cam.enableMouseInput();
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
        int n = referencePoints.size();
        for(int i = 0; i < n; i++) {
            referenceObjectPoints[0].push_back(toCv(referencePoints.getObjectPosition(i)));
            referenceImagePoints[0].push_back(toCv(referencePoints.getImagePosition(i)));
        }
        const static int minPoints = 6;
        if(referenceObjectPoints[0].size() >= minPoints) {
            calibrateCamera(referenceObjectPoints, referenceImagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, CV_CALIB_USE_INTRINSIC_GUESS);
            rvec = rvecs[0];
            tvec = tvecs[0];
            intrinsics.setup(cameraMatrix, imageSize);
            modelMatrix = makeMatrix(rvec, tvec);
            calibrationReady = true;
        } else {
            calibrationReady = false;
        }
    }
};

int main() {
    ofSetupOpenGL(1280, 720, OF_WINDOW);
    ofRunApp(new ofApp());
}