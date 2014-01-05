#pragma once

#include "SelectablePoints.h"
#include "ReferencePoints.h"

class DraggablePoints : public SelectablePoints {
protected:
	ofVec2f mouseStart;
	
	void cachePositions() {
		for(set<unsigned int>::iterator itr = selected.begin(); itr != selected.end(); itr++) {
			points[*itr].positionStart = points[*itr].position;
		}
	}
	static bool isDirectionKey(int key) {
		return
		key == OF_KEY_LEFT ||
		key == OF_KEY_RIGHT ||
		key == OF_KEY_UP ||
		key == OF_KEY_DOWN;
	}
	static ofVec2f getDirectionFromKey(int key) {
		switch(key) {
			case OF_KEY_LEFT: return ofVec2f(-1, 0);
			case OF_KEY_RIGHT: return ofVec2f(+1, 0);
			case OF_KEY_UP: return ofVec2f(0, -1);
			case OF_KEY_DOWN: return ofVec2f(0, +1);
		}
		return ofVec2f(0, 0);
	}
	
public:
	void mousePressed(ofMouseEventArgs& mouse) {
        cout<<"mousePressed"<<endl;
		SelectablePoints::mousePressed(mouse);
		mouseStart = mouse;
		cachePositions();
	}
	void mouseDragged(ofMouseEventArgs& mouse) {
        cout<<"mouseDragged"<<endl;
		ofVec2f offset = mouse - mouseStart;
		for(set<unsigned int>::iterator itr = selected.begin(); itr != selected.end(); itr++) {
			points[*itr].position = points[*itr].positionStart + offset;
		}
	}
	void mouseReleased(ofMouseEventArgs& mouse) {
		cachePositions();
	}
	void keyPressed(ofKeyEventArgs& key) {
		if(isDirectionKey(key.key)) {
			float multiplier = ofGetKeyPressed(OF_KEY_COMMAND) ? .25 : 1;
			ofVec2f offset = multiplier * getDirectionFromKey(key.key);
			for(set<unsigned int>::iterator itr = selected.begin(); itr != selected.end(); itr++) {
				points[*itr].position += offset;
			}
		}
	}
    
    ofVec2f getImagePosition(int i){
        if(points.size() > i)
            return points[i].position;
        else
            return ofVec2f(0, 0);
    }
    
    ofVec3f getObjectPosition(int i){
        if(points.size() > i)
            return points[i].modelPoint;
        else
            return ofVec2f(0, 0);
    }
    
    void add(const ofVec2f& img, const ofVec3f& obj) {
		points.push_back(DraggablePoint());
		points.back().position = img;
        points.back().modelPoint = obj;
	}
    void add(const DraggablePoint& p){
        points.push_back(p);
    }
};