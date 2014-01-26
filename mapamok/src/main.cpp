#include "ofMain.h"
#include "ofxAssimpModelLoader.h"
#include "ofxUI.h"
#include "DraggablePoints.h"
#include "MeshUtils.h"
#include "ofxCv.h"
#include "ofAutoShader.h"
using namespace ofxCv;
using namespace cv;

class ofApp : public ofBaseApp {
public:
    
    enum {
		RENDER_MODE_WIREFRAME_OCCLUDED = 0,
		RENDER_MODE_WIREFRAME_FULL,
		RENDER_MODE_OUTLINE,
		RENDER_MODE_FACES,
	};
    
    
	ofxUICanvas* gui;
	ofxUIRadio* renderMode;
	
	bool editToggle = false;
    bool moveModel = false;
    bool clear = false;
	bool loadButton = false;
	bool saveButton = false;
	bool useShader = false;
    bool useSphereLights = false;
    bool useLightShader = false;
    bool calibrationReady = false;
    bool objectLoaded = false;
    bool cullBack = false;
    bool validShader = false;
    bool fullScreen = false;
    float backgroundBrightness = 0;
    float aov = 0.80;
    
    ofxAssimpModelLoader model;
	ofMesh mesh;
	ofMesh cornerMesh, imageMesh;
    ofMesh calibrationMesh;
    ofMesh refMesh;
    ofMesh object;
    ofVboMesh objectVbo;
    ofLight light;
    
	ofEasyCam cam;
	DraggablePoints objectPoints;
    cv::Mat rvec, tvec;
    ofMatrix4x4 modelMatrix;
    ofxCv::Intrinsics intrinsics;
    
    ofAutoShader shader;
    ofAutoShader sphereShader;
    
    string modelFileName;
    ofVec2f mousePoint;
    ofRectangle fooRect;
    
    //Sphere Lights
    vector<float> lightRadii;
	vector<ofVec3f> lightPositions;
    ofIcoSpherePrimitive sphere;
    
	void setup() {
		ofSetWindowTitle("mapamok");
        ofSetVerticalSync(true);
        
		setupGui();
		if(ofFile::doesFileExist("model.dae")) {
			loadModel("model.dae");
		}
		cam.setDistance(1);
		cam.setNearClip(.1);
		cam.setFarClip(10);
        
        fooRect = ofRectangle(ofGetWidth()-100, ofGetHeight()-100, 100, 100);
        
        light.setPosition(0, 0, 0);
        
        shader.setup("shaders/shader");
        sphereShader.setup("shaders/sphere");
        
        
        int n = 12;
		lightPositions.resize(n);
		for(int i = 0; i < n; i++) {
			lightRadii.push_back(ofRandom(1, 30));
		}
        
        sphere.setMode(OF_PRIMITIVE_POINTS);
		sphere.set(1, 3);
	}
    
    
    ofVec3f noise3d(float x, float y) {
        return ofVec3f(ofGetWidth()/2+sin(x),
                       ofGetHeight()/2+cos(y),
                       sin(x*y));
    }
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
        gui->addToggle("Move Model", &moveModel);
		gui->addToggle("Edit", &editToggle);
		gui->addButton("Load", &loadButton);
		gui->addToggle("Save", &saveButton);
        gui->addButton("Clear Calibration", &clear);
        
		
		gui->addSpacer();
		gui->addLabel("Render");
		vector<string> renderModes;
		renderModes.push_back("Occluded wireframe");
		renderModes.push_back("Full wireframe");
		renderModes.push_back("Depth Edges");
		renderModes.push_back("Faces");
		renderMode = gui->addRadio("Render", renderModes, OFX_UI_ORIENTATION_VERTICAL, OFX_UI_FONT_MEDIUM);
		renderMode->activateToggle(renderModes[0]);
        gui->addToggle("Cull Back", &cullBack);
		
		gui->addSpacer();
		gui->addMinimalSlider("Background", 0, 255, &backgroundBrightness);
		gui->addToggle("Shader", &useShader);
        gui->addToggle("Sphere Lights", &useSphereLights);
        gui->addToggle("Light Shader", &useLightShader);
		
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
		
        if(calibrationReady){
            drawCalibrated();
        }else if(editToggle){
            drawSetup();
        }
        if(!calibrationReady || clear){
            drawCalibrate();
            drawMouse();
        }
        
        
        int n = lightPositions.size();
        for(int i = 0; i < n; i++) {
			ofPushMatrix();
			ofTranslate(lightPositions[i]);
			ofScale(lightRadii[i], lightRadii[i], lightRadii[i]);
			sphere.draw();
			ofPopMatrix();
		}
	}
    
    void drawCalibrate(ofRectangle foo = ofGetWindowRect()){
        ofPushStyle();
        {
            ofEnableAlphaBlending();
            ofSetColor(ofColor::magenta, 100);
            if(moveModel){
                objectPoints.clear();
                calibrationMesh.clear();
                
                object = mesh;
                project(object, cam, ofGetWindowRect());
                objectVbo = object;
                
                cornerMesh = refMesh;
                project(cornerMesh, cam, ofGetWindowRect());
                
                float totalPoints = cornerMesh.getNumVertices();
                float numPoints = totalPoints;
                float cutOff = 1.0;
                while(numPoints > 200 && cutOff > 0){
                    numPoints = totalPoints*cutOff;
                    cutOff -= 0.1;
                }
                for(int i = 0 ; i < totalPoints; i+=totalPoints/numPoints){
                    objectPoints.add(cornerMesh.getVertices()[i], object.getVertices()[i]);
                }
            }
            ofEnableDepthTest();
            imageMesh.draw();
            ofDisableDepthTest();
            ofDisableAlphaBlending();
        }
        ofPopStyle();
    }
    void drawMouse(){
        ofEnableAlphaBlending();
        ofPushStyle();
        {
            ofSetLineWidth(1);
            ofSetColor(255, 0, 255, 100);
            ofLine(ofGetMouseX(), 0, ofGetMouseX(), ofGetWindowHeight());
            ofSetColor(255,255, 0, 100);
            ofLine(0, ofGetMouseY(), ofGetWindowWidth(), ofGetMouseY());
        }
        ofPopStyle();
    }
    
    void drawSetup(ofRectangle foo = ofGetWindowRect()){
        cam.begin(foo);
        ofSetLineWidth(2);
        ofSetColor(ofColor::blueViolet);
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
        }
        
        
        
        ofEnableDepthTest();
        float pointSize = 2;
        glPointSize(pointSize);
        glEnable(GL_POLYGON_OFFSET_POINT);
        glPolygonOffset(-pointSize, -pointSize);
        ofSetColor(ofColor::white);
        calibrationMesh.drawVertices();
        glDisable(GL_POLYGON_OFFSET_POINT);
        ofDisableDepthTest();
        
        cam.end();
        
        
    }
    
    void update(){
        int n = lightPositions.size();
		float t = ofGetElapsedTimef() * .05;
		for(int i = 0; i < n; i++) {
			lightPositions[i] = noise3d(i, t);
			lightPositions[i] *= 256;
		}
        if(clear){
            cornerMesh = mesh;
            cornerMesh.clearIndices();
            cornerMesh.setMode(OF_PRIMITIVE_POINTS);
            cornerMesh.addIndices(getRankedCorners(mesh));
            
            object = mesh;
            project(object, cam, ofGetWindowRect());
            objectVbo = object;
            refMesh = cornerMesh;
            project(cornerMesh, cam, ofGetWindowRect());
            
            
            float totalPoints = cornerMesh.getNumVertices();
            float numPoints = totalPoints;
            float cutOff = 1.0;
            while(numPoints > 200 && cutOff > 0){
                numPoints = totalPoints*cutOff;
                cutOff -= 0.001;
            }
            for(int i = 0 ; i < totalPoints; i+=numPoints){
                objectPoints.add(cornerMesh.getVertices()[i], object.getVertices()[i]);
                calibrationMesh.addVertex(cornerMesh.getVertices()[i]);
            }
            objectPoints.isCalibrated(calibrationReady);
            calibrationReady = false;
            clear = false;
        }
        if(calibrationReady && saveButton){
            save();
            saveButton = false;
        }
    }
    
    void drawShaderDebug(){
        ofPushStyle();
		ofSetColor(ofColor::magenta);
		ofSetLineWidth(8);
		ofLine(0, 0, ofGetWidth(), ofGetHeight());
		ofLine(ofGetWidth(), 0, 0, ofGetHeight());
		string message = "Shader failed to compile.";
		ofVec2f center(ofGetWidth(), ofGetHeight());
		center /= 2;
		center.x -= message.size() * 8 / 2;
		center.y -= 8;
		drawHighlightString(message, center);
		ofPopStyle();
    }
    
    
    void drawCalibrated(){
        ofPushStyle();
        ofPushMatrix();
        glPushMatrix();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        
        intrinsics.loadProjectionMatrix(10, 2000);
        applyMatrix(modelMatrix);
        
        
        if(!editToggle){
            
            
            if(useShader) {
                glPushAttrib(GL_ALL_ATTRIB_BITS);
                glEnable(GL_DEPTH_TEST);
                if(useShader) {
                    shader.begin();
                    shader.setUniform1f("elapsedTime", ofGetElapsedTimef());
                    shader.end();
                }
                if(useLightShader){
                    
                }
                if(useSphereLights){
                    sphereShader.begin();
                    sphereShader.setUniform1f("diffuseLight", ofMap(sin(ofGetElapsedTimef()/60), -1, 1, 0, 1));
                    sphereShader.setUniform1i("numLights", lightPositions.size());
                    sphereShader.setUniform3fv("lightPositions", (float*) &(lightPositions[0]), lightPositions.size());
                    sphereShader.setUniform1fv("lightRadii", (float*) &(lightRadii[0]), lightRadii.size());
                    sphereShader.end();
                }
            }
            
            ofSetLineWidth(2);
            int renderModeSelection = getSelection(renderMode);
            if(renderModeSelection == RENDER_MODE_FACES) {
                if(useShader) shader.begin();
                if(useSphereLights)sphereShader.begin();
                if(cullBack){
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                }
                ofEnableDepthTest();
                object.drawFaces();
                ofDisableDepthTest();
                if(useSphereLights)sphereShader.end();
                if(useShader) shader.end();
                if(cullBack){
                    glDisable(GL_CULL_FACE);
                }
            } else if(renderModeSelection == RENDER_MODE_WIREFRAME_FULL) {
                if(useShader) shader.begin();
                if(useSphereLights)sphereShader.begin();
                object.drawWireframe();
                if(useSphereLights)sphereShader.end();
                if(useShader) shader.end();
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
                if(useShader) shader.begin();
                if(useSphereLights)sphereShader.begin();
                object.drawFaces();
                glColorMask(true, true, true, true);
                glDisable(GL_POLYGON_OFFSET_FILL);
                object.drawWireframe();
                if(useSphereLights)sphereShader.end();
                if(useShader) shader.end();
                prepareRender(false, false, false);
            }
            
        }else{
            ofEnableDepthTest();
            float pointSize = 4;
            glPointSize(pointSize);
            ofSetColor(ofColor::green);
            glEnable(GL_POLYGON_OFFSET_POINT);
            glPolygonOffset(-pointSize, -pointSize);
            refMesh.drawVertices();
            ofSetColor(ofColor::yellow);
            calibrationMesh.drawVertices();
            glDisable(GL_POLYGON_OFFSET_POINT);
            ofDisableDepthTest();
        }
        
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        ofPopMatrix();
        ofPopStyle();
    }
    
    
	void loadModel(string filename) {
        modelFileName = filename;
        
		model.loadModel(modelFileName);
		mesh = collapseModel(model);
		centerAndNormalize(mesh);
		
        if(ofIsStringInString(modelFileName, ".dae"))
            ofStringReplace(modelFileName, ".dae", "");
        if(ofIsStringInString(modelFileName, ".ply"))
            ofStringReplace(modelFileName, ".ply", "");
        
        if(loadCalibration(modelFileName)){
            object.load(modelFileName+"-mesh");
            cornerMesh.load(modelFileName+"-corner");
            
            
            refMesh = mesh;
            refMesh.clearIndices();
            refMesh.setMode(OF_PRIMITIVE_POINTS);
            refMesh.addIndices(getRankedCorners(mesh));
        }else{
            cornerMesh = mesh;
            cornerMesh.clearIndices();
            cornerMesh.setMode(OF_PRIMITIVE_POINTS);
            cornerMesh.addIndices(getRankedCorners(mesh));
            
            object = mesh;
            project(object, cam, ofGetWindowRect());
            objectVbo = object;
            refMesh = cornerMesh;
            project(cornerMesh, cam, ofGetWindowRect());
        }
        
        
        float totalPoints = cornerMesh.getNumVertices();
        float numPoints = totalPoints;
        float cutOff = 1.0;
        while(numPoints > 200 && cutOff > 0.01){
            numPoints = totalPoints*cutOff;
            cutOff -= 0.001;
        }
        for(int i = 0 ; i < totalPoints; i++){
            objectPoints.add(cornerMesh.getVertices()[i], object.getVertices()[i]);
            calibrationMesh.addVertex(cornerMesh.getVertices()[i]);
        }
        
        objectPoints.setClickRadius(2);
        objectPoints.enableControlEvents();
        objectPoints.enableDrawEvent();
        
        objectLoaded = true;
	}
    
    bool loadCalibration(string modelname, ofRectangle viewport = ofGetWindowRect()){
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
        
        return calibrationReady;
    }
    
    void keyPressed(int key){
        if(key == ' ')
            updateCalibration(ofGetWindowRect());
        if(key == OF_KEY_CONTROL)
            objectPoints.toggleDrawEvents();
    }
    
    void updateCalibration(ofRectangle viewport = ofGetWindowRect()) {
        
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
        vector<ofVec3f> modelPoints = objectPoints.getModelPoints();
        vector<ofVec2f> selectedPoints = objectPoints.getSelectedPoints();
        
        
        
        int n = selectedPoints.size();
        for(int i = 0; i < n; i++) {
            referenceObjectPoints[0].push_back(toCv(modelPoints[i]));
            referenceImagePoints[0].push_back(toCv(selectedPoints[i]));
        }
        const static int minPoints = 6;
        if(referenceObjectPoints[0].size() >= minPoints) {
            refMesh.clear();
            refMesh.addVertices(modelPoints);
            
            calibrationMesh.clear();
            calibrationMesh.addVertices(modelPoints);
            
            calibrateCamera(referenceObjectPoints, referenceImagePoints, imageSize, cameraMatrix, distCoeffs, rvecs, tvecs, CV_CALIB_USE_INTRINSIC_GUESS);
            rvec = rvecs[0];
            tvec = tvecs[0];
            intrinsics.setup(cameraMatrix, imageSize);
            modelMatrix = makeMatrix(rvec, tvec);
            calibrationReady = true;
        } else {
            calibrationReady = false;
        }
        
        objectPoints.isCalibrated(calibrationReady);
    }
    
    void mouseMoved(int x, int y){
        mousePoint.set(x, y);
    }
    
	void dragEvent(ofDragInfo dragInfo) {
		if(dragInfo.files.size() == 1) {
			string filename = dragInfo.files[0];
            loadModel(filename);
		}
	}
    
    void exit(){
        if(calibrationReady)
            save();
    }
    
    void save(){
        if(objectLoaded){
            saveMat(rvec, modelFileName+"-rotation-calibration");
            saveMat(tvec, modelFileName+"-translation-calibration");
            object.save(modelFileName+"-mesh");
            cornerMesh.save(modelFileName+"-corner");
        }
    }
};

int main() {
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp());
}