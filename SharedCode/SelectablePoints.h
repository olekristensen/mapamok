#pragma once

#include "EventWatcher.h"
#include "DraggablePoint.h"

class SelectablePoints : public EventWatcher {
protected:
	vector<DraggablePoint> points;
    set<unsigned int> calibrated;
	set<unsigned int> selected;
    bool bCalibrated;
	
	float pointSize, clickRadiusSquared;
	
public:
	SelectablePoints()
	:clickRadiusSquared(0) {
	}
	unsigned int size() {
		return points.size();
	}
	void add(const ofVec2f& v) {
		points.push_back(DraggablePoint());
		points.back().position = v;
	}

    void add(const ofVec2f& img, const ofVec3f& obj) {
        points.push_back(DraggablePoint());
        points.back().position = img;
        points.back().modelPosition = obj;
    }
	void setClickRadius(float clickRadius) {
		this->clickRadiusSquared = clickRadius * clickRadius;
	}
    
    ofVec2f getImagePosition(int i){
        if(points.size() > i)
            return points[i].position;
        else
            return ofVec2f(0, 0);
    }
    ofVec3f getObjectPosition(int i){
        if(points.size() > i)
            return points[i].modelPosition;
        else
            return ofVec2f(0, 0);
    }
    
    vector<ofVec2f> getSelectedPoints(){
        vector<ofVec2f> foo;
        for(set<unsigned int>::iterator itr = calibrated.begin(); itr != calibrated.end(); ++itr) {
            foo.push_back(points[*itr].position);
        }
        return foo;
    }
    
    vector<ofVec3f> getSelectedModelPoints(){
        vector<ofVec3f> foo;
        for(set<unsigned int>::iterator itr = calibrated.begin(); itr != calibrated.end(); ++itr) {
            foo.push_back(points[*itr].position);
        }
        return foo;
    }
    
    vector<ofVec3f> getModelPoints(){
        vector<ofVec3f> foo;
        for(set<unsigned int>::iterator itr = calibrated.begin(); itr != calibrated.end(); ++itr) {
            foo.push_back(points[*itr].modelPosition);
        }
        return foo;
    }
    
    void isCalibrated(bool foo){
        bCalibrated = foo;
    }
    
	void mousePressed(ofMouseEventArgs& mouse) {
		bool shift = ofGetKeyPressed(OF_KEY_SHIFT);
		bool hitAny = false;
		for(int i = 0; i < size(); i++) {
			bool hit = points[i].isHit(mouse, clickRadiusSquared);
			if(hit && !hitAny) {
				if(!points[i].selected) {
					points[i].selected = true;
					selected.insert(i);
					hitAny = true;
				}
			} else if(!shift) {
				points[i].selected = false;
				selected.erase(i);
			}
		}
	}
	void draw(ofEventArgs& args) {
        if(!bCalibrated){
            for(int i = 0; i < size(); i++) {
                points[i].draw(clickRadiusSquared);
            }
        }else{
            for(set<unsigned int>::iterator itr = calibrated.begin(); itr != calibrated.end(); ++itr) {
                points[*itr].draw(clickRadiusSquared);
            }
        }
	}
    
    void clear(){
        points.clear();
        calibrated.clear();
        selected.clear();
    }
};