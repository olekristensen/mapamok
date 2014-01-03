#pragma once

class DraggablePoint {
public:
	ofVec2f position, positionStart;
    ofVec3f modelPoint;
	bool selected, dragging;
	
	DraggablePoint()
	:selected(false)
	,dragging(false) {
	}
	bool isHit(ofVec2f v, float clickRadiusSquared) {
        cout<<"isHit"<<endl;
		return position.distanceSquared(v) < clickRadiusSquared;
	}
	void draw(float clickRadiusSquared) {
		float r = sqrt(clickRadiusSquared);
		ofPushStyle();
		ofNoFill();
		ofSetLineWidth(2);
		if(selected) {
			ofSetColor(ofColor::yellow);
			ofCircle(position, r + 4);
		}
		ofPopStyle();
		ofPushStyle();
		ofFill();
        ofSetColor(ofColor::magenta);
		ofCircle(position, r);
		ofPopStyle();
	}
};