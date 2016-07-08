//
//  Layer.cpp
//  TerraForma
//
//  Created by Tyler on 6/19/16.
//
//

#include "Layer.hpp"

void Layer::loadVideo(string videoFile){
    
    vid.load(videoFile);
    vidW = vid.getWidth();
    vidH = vid.getHeight();
    vid.play();
}

void Layer::setThresholds(int near, int far){
    
    nearThresh = near;
    farThresh = far;
}

void Layer::setColor(ofColor tint){
    color = tint;
}

void Layer::update(ofImage& depthImg){
    
    vid.update();
    
    if (vid.isFrameNew()){
        frame.setFromPixels(vid.getPixels());
        threshold(depthImg);
        vid.setVolume(min(pctShown*2.0,1.0));
    }
}

void Layer::threshold(ofImage& depthImg){

    Mat depthMat = toCv(depthImg); // convert kinect depth img to cv::Mat
    
    // threshold top first
    cv::threshold(depthMat, maskMat, nearThresh, 255, cv::THRESH_TOZERO_INV);
    // threshold the bottom
    cv::threshold(maskMat, maskMat, farThresh, 255, cv::THRESH_BINARY);
    
    // calc % of white pixels to black pixels
    int nPix = maskMat.rows * maskMat.cols;
    int nShown = countNonZero(maskMat);
    pctShown = (float)nShown / (float)nPix;
    
    cv::resize(maskMat, maskMat, cv::Size(vidW,vidH)); // resize to video dims
    
    Mat frameMat = toCv(frame); // wrap video frame as Mat
    Mat layerMat(frameMat.rows,frameMat.cols,CV_8UC4);
    
    // mask frame to alpha
    
    Mat rgb[3];
    split(frameMat,rgb);     // split frame into rgb channels
    Mat rgba[4]={rgb[0],rgb[1],rgb[2],maskMat};     // copy mask to alpha channel
    merge(rgba,4,layerMat);     // merge 4 channels back to layerMat
    
    layer = ofImage(); // reset layer
    toOf(layerMat,layer); // convert Mat to ofImage
    
}

void Layer::draw(float x, float y, float w, float h){
    ofPushStyle();
    ofSetColor(color);
    
    layer.draw(x,y,w,h);
    
    ofPopStyle();
}

void Layer::drawFrame(float x, float y, float w, float h){
    frame.draw(x,y,w,h);
}

void Layer::drawMask(float x, float y, float w, float h){
    drawMat(maskMat,x,y,w,h);
}

