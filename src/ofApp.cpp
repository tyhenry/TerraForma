#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    kinect.setRegistration(false);
    kinect.init();
    kinect.open();
    kinect.setDepthClipping(950,1350); // kinect depth calcs 95cm-135cm (higher resolution)
    
    ofBackground(0);
    ofSetVerticalSync(true);
    
    layers.resize(4);
    layers[0].loadVideo("mars 1080.mp4");
    layers[0].setThresholds(255, THRESH0); // near, far
    layers[0].setColor(ofColor(236,109,18));
    ofDirectory dir0("mars audio");
    dir0.allowExt("aif");
    dir0.listDir();
    for (int i=0; i<dir0.size(); i++){
        layers[0].loadAudio(dir0.getPath(i));
    }
    
    layers[1].setApache();
    layers[1].loadVideo("apache 1080.mp4");
    layers[1].setThresholds(THRESH0, THRESH1);
    layers[1].setColor(ofColor(255,225,86));
    layers[1].loadAudio("apache ambient.aif");
    
    layers[2].loadVideo("fracking 1080.mp4");
    layers[2].setThresholds(THRESH1, THRESH2);
    layers[2].setColor(ofColor(137,210,65));
    ofDirectory dir2("fracking audio");
    dir2.allowExt("aif");
    dir2.listDir();
    for (int i=0; i<dir2.size(); i++){
        layers[2].loadAudio(dir2.getPath(i));
    }
    
    layers[3].loadVideo("piri reis 1080.mp4");
    layers[3].setThresholds(THRESH2, 0);
    layers[3].setColor(ofColor(89,192,214));
    
    warper.setup(0,0,640,480);
    fbo.allocate(640,480,GL_RGB);
    
    width = ofGetWidth();
    height = ofGetHeight();
    
    depthGui.setup();
    depthGui.add(topSlider.setup("top", TOP, 0, 255));
    depthGui.add(thresh0Slider.setup("thresh0", THRESH0, 0, 255));
    depthGui.add(thresh1Slider.setup("thresh1", THRESH1, 0, 255));
    depthGui.add(thresh2Slider.setup("thresh2", THRESH2, 0, 255));
    depthGui.add(bottomSlider.setup("bottom", TOP, 0, 255));
    depthGui.add(blurRadius.setup("blur", 4, 0, 40));
    depthGui.add(lerpAmount.setup("smooth", 0.01, 0, 1.0));

    // set kinect warp corners to defaults
    kinectCorners[TOP_LEFT] = ofVec2f(0,0);
    kinectCorners[TOP_RIGHT] = ofVec2f(640,0);
    kinectCorners[BOTTOM_RIGHT] = ofVec2f(640,480);
    kinectCorners[BOTTOM_LEFT] = ofVec2f(0,480);
    
    kinectWarp.allocate(640,480,OF_IMAGE_COLOR);
    kinectWarp.setColor(ofColor(0,0,0));
    kinectWarp.setImageType(OF_IMAGE_GRAYSCALE);
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    kinect.update();
    
    ofSoundUpdate();
    
    if (kinect.isFrameNewDepth()){
        
        // auto-set crop/warp of kinect image to projection size if not done yet
        if (!bWarped && ofGetElapsedTimef() > 10){
            autoSetKinectWarp();
        }
        
        // quad-warp new kinect frame to match projection
        warpKinect();
    }
    
    
     // update each layer with depth image for thresholding
    
    for (int l=0; l<layers.size(); l++){
        layers[l].update(kinectWarp);
    }
    
    playSoundbites(); // plays mixed audio tracks according to time and rate of layer visibility
    
    
    // calibrate depth mode - sets thresholds based on GUI
    
    if (mode == CALIBRATEDEPTH){
        layers[0].setThresholds(topSlider, thresh0Slider);
        layers[1].setThresholds(thresh0Slider, thresh1Slider);
        layers[2].setThresholds(thresh1Slider, thresh2Slider);
        layers[3].setThresholds(thresh2Slider, bottomSlider);
    } 
    
    // kinect image registration/warping mode
    
    if (mode == AUTOKINECT){
        autoSetKinectWarp();
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    switch (mode){
            
        case CALIBRATEDEPTH: // depth threshold calibration mode (with gui)
        {
            float w = width*0.5; float h = height*0.5;
            
            layers[0].draw(0,0,w,h);
            layers[1].draw(w,0,w,h);
            layers[2].draw(0,h,w,h);
            layers[3].draw(w,h,w,h);
            
            // draw gui to adjust depth of each layer threshold
            depthGui.draw();
            
            // draw percentage shown of each layer
            ofSetColor(0);
            ofDrawRectangle(0,height-100,width*0.5,100);
            ofSetColor(255);
            ofPushMatrix();
            ofTranslate(10,height-90);
            for (int l=0; l<4; l++){
                ofTranslate(0,20);
                string pct = "% of layer " + ofToString(l) + ": " + ofToString(layers[l].pctShown);
                ofDrawBitmapString(pct, 0,0);
            }
            ofPopMatrix();
            break;
        }
            
        case KINECT: // manual kinect warping mode - shows original kinect depth img and warp result
        {
            kinect.drawDepth(0,0,640,480);
            kinectWarp.draw(650,0,width-660,(width-660)/640*480);
            for (int i=0; i<4; i++){
                if (cornerSelect == i) ofSetColor(0,255,0);
                ofDrawCircle(kinectCorners[i], 5);
                ofSetColor(255);
            }
            break;
        }
            
        case AUTOKINECT: // auto kinect warping mode - shows warped kinect img and CV estimation results
        {
            ofPushMatrix();
            ofScale(width/640,height/480);
            
            kinectRaw.draw(0,0);
            contourFinder.draw();
            
            ofSetColor(0,255,0);
            for (int p=0; p<4; p++){
                ofDrawCircle(kinectCorners[p].x,kinectCorners[p].y,5);
            }
            ofSetColor(255);
            
            ofPopMatrix();
            break;
        }
            
            
        // normal play mode!
    
        default:
        {
            for (int l=layers.size()-1; l>=0; l--){ // draw layers bottom to top
                layers[l].draw(0,0,width,height);
            }
            break;
        }
    }
}

//--------------------------------------------------------------
void ofApp::warpKinect(){
    pKinectWarp = kinectWarp;
    
    kinectRaw.setFromPixels(kinect.getDepthPixels(), 640,480, OF_IMAGE_GRAYSCALE);
    
    fbo.begin();
    ofClear(255);
    
    warper.begin();
    kinectRaw.draw(0,0,640,480);
    warper.end();
    
    fbo.end();
    
    ofPixels fboPix;
    fbo.readToPixels(fboPix);
    kinectWarp.setFromPixels(fboPix);
    kinectWarp.update();
    if (blurRadius > 0){
        ofxCv::GaussianBlur(kinectWarp, blurRadius);
    }
    kinectWarp.setImageType(OF_IMAGE_GRAYSCALE);
    ofxCv::lerp(kinectWarp,pKinectWarp,kinectWarp); // smooth
    
}

//--------------------------------------------------------------
void ofApp::autoSetKinectWarp(){
    
    // uses openCV to find bounding box of sandbox, and then the inset corners
    // these becomes warp coordinates for Kinect<->projection quad-warp registration
    
    contourFinder.setMinAreaRadius(150);
    contourFinder.setMaxAreaRadius(640);
    contourFinder.setThreshold(140);
    contourFinder.setFindHoles(false);
    contourFinder.setSortBySize(true);
    contourFinder.findContours(kinectRaw);
    
    vector<cv::Point> quad = contourFinder.getFitQuad(contourFinder.size()-1);
    
    // save to warp coords
    
    ofPolyline poly;
    for (int p=0; p<4; p++){
        poly.addVertex(toOf(quad[p]));
    }
    
    // sort clockwise
    kinectCorners[TOP_LEFT] = poly.getClosestPoint(ofVec2f(0,0)) + ofVec2f(10,10);
    kinectCorners[TOP_RIGHT] = poly.getClosestPoint(ofVec2f(640,0)) + ofVec2f(-10,10);
    kinectCorners[BOTTOM_RIGHT] = poly.getClosestPoint(ofVec2f(640,480)) + ofVec2f(-10,-10);
    kinectCorners[BOTTOM_LEFT] = poly.getClosestPoint(ofVec2f(0,480)) + ofVec2f(10,-10);
    
    for (int p=0; p<4; p++) {
        
        // convert kinect image crop coords to warp coords
        ofVec2f corner = kinectCorners[p];

        switch (p){
            case 0:
                corner.set(0-corner.x,0-corner.y);
                break;
            case 1:
                corner.set(1280-corner.x,0-corner.y);
                break;
            case 2:
                corner.set(1280-corner.x,960-corner.y);
                break;
            case 3:
                corner.set(0-corner.x,960-corner.y);
                break;
        }
        warper.setCorner(ofxGLWarper::CornerLocation(p),corner);
    }
    
    bWarped = true;
    
}

//--------------------------------------------------------------
void ofApp::playSoundbites(){
    
    
    // if we've waited enough time...
    
    if (ofGetElapsedTimef() - lastSoundbiteTime >= timeBtwnSoundbites){
        
        ofLogNotice("soundbites") << "triggering new sound...";
        
        // get current layer visibility percentages
        
        vector<float> cLayersPctShown;
        
        for (int i=0; i<3; i++){ // don't use layer 3, only 0-2
            
            cLayersPctShown.push_back(layers[i].pctShown);
        }
        // if this is the first time we've tracked, save now
        if (layersPctShown.size() != cLayersPctShown.size()){
            layersPctShown = cLayersPctShown;
        }
        
        // compare to last soundbite's percentages
        // whichever layer increased most, trigger that one
        int bestLayer = 0;
        float largestDiff = 0.0;
        for (int i=0; i<cLayersPctShown.size(); i++){
            float diff = cLayersPctShown[i] - layersPctShown[i];
            if (diff >= largestDiff) {
                largestDiff = diff;
                bestLayer = i; // save layer with largest increase
            }
        }
        
        ofLogNotice("soundbites") << "chose layer " << bestLayer << " with % change: " << largestDiff;
        
        // if largest increase was less than 5%
        // weighted random choice of soundbite track
        if (largestDiff <= 0.01) {
            
            // random choice of layer with weighted probability
//            float rnd = ofRandom(1); if (rnd==1) rnd = 0.9999;
//            for (int i=0; i<cLayersPctShown.size(); i++){
//                if (cLayersPctShown[i] > rnd){
//                    bestLayer = i;
//                } else {
//                    rnd -= cLayersPctShown[i];
//                }
//            }
            int rnd = ofRandom(0,3); if (rnd==3) rnd = 2;
            bestLayer = rnd;
            ofLogNotice("soundbites") << "randomly chose layer " << bestLayer << " with rnd of " << rnd;

        }
        
        
        layers[bestLayer].triggerSound(); // we play the sound
               
        layersPctShown = cLayersPctShown; // save layers % for next time
        lastSoundbiteTime = ofGetElapsedTimef(); // save time for wait calc
        
        timeBtwnSoundbites = ofRandom(5, 15); // new wait time for next round
        
        ofLogNotice("soundbites") << "played sound from layer " << bestLayer << ", will now wait " << timeBtwnSoundbites << " seconds";
        
    }
    
    
}


//--------------------------------------------------------------
void ofApp::exit(){
    
    kinect.close();
    
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if (key==' '){
        mode = NORMAL;
    }
    else if (key=='d'){
        if (mode == CALIBRATEDEPTH) mode = NORMAL;
        else mode = CALIBRATEDEPTH;
    }
    if (key=='k'){
        if (mode == KINECT) mode = NORMAL;
        else mode = KINECT;
    }
    else if (key=='a'){
        if (mode == AUTOKINECT) mode = NORMAL;
        else mode = AUTOKINECT;
    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
    if (mode == KINECT){
        kinectCorners[cornerSelect].set(x,y);
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
    if (mode == KINECT){
        ofVec2f m(x,y);
        for (int i=0; i<4; i++){
            if (kinectCorners[i].distance(m) <= 8){
                cornerSelect = i;
                break;
            }
        }
    }

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
    if (mode == KINECT){
        ofVec2f corner(kinectCorners[cornerSelect].x,kinectCorners[cornerSelect].y);
        // convert kinect image crop coords to warp coords
        switch (cornerSelect){
            case TOP_LEFT:
                corner.set(0-corner.x,0-corner.y);
                break;
            case TOP_RIGHT:
                corner.set(1280-corner.x,0-corner.y);
                break;
            case BOTTOM_RIGHT:
                corner.set(1280-corner.x,960-corner.y);
                break;
            case BOTTOM_LEFT:
                corner.set(0-corner.x,960-corner.y);
                break;
        }
        warper.setCorner(ofxGLWarper::CornerLocation(cornerSelect),corner);
    }

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
