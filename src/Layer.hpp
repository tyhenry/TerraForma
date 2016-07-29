//
//  Layer.hpp
//  TerraForma
//
//  Created by Tyler on 6/19/16.
//
//

#pragma once
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"

using namespace cv;
using namespace ofxCv;

class Layer {
    
public:
    
    Layer(){}
    void loadVideo(string videoFile);
    void loadAudio(string audioFile);
    void setThresholds(int near, int far);
    void setColor(ofColor tint);
    void setApache(bool apache=true) { bApache = apache; }
    
    void update(ofImage& depthImg);
    void draw(float x, float y, float w, float h);
    
    void triggerSound();
    
    void drawFrame(float x, float y, float w, float h);
    void drawMask(float x, float y, float w, float h);
    
    void threshold(ofImage& depthImg);
    
    ofVideoPlayer vid;
    ofImage frame; // current video frame
    ofImage layer; // masked video frame
    Mat maskMat; // mask
    
    int nearThresh = 255; int farThresh = 0;
    float pctShown = 1.0; // tracks pct of video to display
    
    int vidW, vidH;
    ofColor color = ofColor::white;
    
    vector<ofSoundPlayer> sounds;
    
    bool bApache = false; // apache video has one sound track in sync with video
    // so triggers alter volume rather than play a sound
    float apacheSoundLength = 8.0; // default 8 seconds clip length
    float apacheSoundTriggerTime = 0; // when was last triggered
};
