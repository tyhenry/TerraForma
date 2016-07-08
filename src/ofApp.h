#pragma once

#include "ofMain.h"
#include "ofxKinect.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGLWarper.h"
#include "ofxGui.h"
#include "Layer.hpp"

using namespace cv;
using namespace ofxCv;

enum Thresh {
    TOP = 255,
    THRESH0 = 150,
    THRESH1 = 115,
    THRESH2 = 80,
    BOTTOM = 0
};

enum Corner {
    TOP_LEFT = 0,
    TOP_RIGHT,
    BOTTOM_RIGHT,
    BOTTOM_LEFT
};

enum Mode {
    NORMAL,
    CALIBRATEDEPTH,
    KINECT,
    AUTOKINECT
};

class ofApp : public ofBaseApp{

public:
    void setup();
    void update();
    void draw();
    void warpKinect();
    void autoSetKinectWarp();
    void exit();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    vector<Layer> layers;
    
    ofxKinect kinect;
    ofImage kinectRaw, kinectWarp, pKinectWarp;
    
    ofxGLWarper warper;
    ofFbo fbo;

    ofVec2f kinectCorners[4];
    int cornerSelect = 0;
    ContourFinder contourFinder;
    
    ofxPanel depthGui;
    ofxIntSlider topSlider;
    ofxIntSlider thresh0Slider;
    ofxIntSlider thresh1Slider;
    ofxIntSlider thresh2Slider;
    ofxIntSlider bottomSlider;
    ofxIntSlider blurRadius;
    ofxFloatSlider lerpAmount;
    
    int mode = NORMAL;
    
    float width, height;
    
    bool bWarped = false;
		
};
