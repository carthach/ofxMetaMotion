#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetFrameRate(200);
    threadedmmc.setup();
    
    // open an outgoing connection to HOST:PORT
    sender.setup(HOST, PORT);    
}

//--------------------------------------------------------------
void ofApp::update()
{
    sendOSCMessages();
}

void ofApp::sendOSCMessages()
{
    {
        ofxOscMessage m;
        m.setAddress("/metawear/0/fusion");
        for(auto i=0; i<3; i++)
            m.addFloatArg(threadedmmc.mmc.angle[i]);
        sender.sendMessage(m, false);
    }
    
    //Conversions done here for compatibility with Gesture Sound Toolkit
    //https://github.com/bcaramiaux/Gestural-Sound-Toolkit
    
    {
        ofxOscMessage m;
        m.setAddress("/metawear/0/acceleration");
        for(auto i=0; i<3; i++)
            m.addFloatArg(threadedmmc.mmc.outputAcceleration[i]);
        sender.sendMessage(m, false);
    }
    
    {
        ofxOscMessage m;
        m.setAddress("/metawear/0/gyro");
        for(auto i=0; i<3; i++)
        {
            auto value = ofMap(threadedmmc.mmc.outputGyro[i], -2000.0, 2000.0, -2, 2);
            m.addFloatArg(value);
        }
        sender.sendMessage(m, false);
    }
    
    {
        ofxOscMessage m;
        m.setAddress("/metawear/0/mag");
        for(auto i=0; i<3; i++)
        {
            // Specs taken from https://www.bosch-sensortec.com/products/motion-sensors/magnetometers/bmm150/
            auto value = threadedmmc.mmc.outputMag[i];
            
            // X/Y
            if(i < 2)
                value = ofMap(value, -1300.0, 1300.0, -1, 1);
            else
                value = ofMap(value, -2500.0, 2500.0, -1, 1);
            
            m.addFloatArg(value);
        }
        sender.sendMessage(m, false);
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofSetBackgroundColor(30);
    
    ofPushMatrix();
    ofNoFill();
    ofTranslate(ofGetWidth()*.5, ofGetHeight()*.5, 0);
    
    // rotate
    float* a = threadedmmc.mmc.getAngle();
    
    float rX = a[2];
    float rY = -a[0];
    float rZ = a[1];
            
    ofRotateXDeg(rX);
    ofRotateYDeg(rY);
    ofRotateZDeg(rZ);

    ofDrawBox(0, 0, 0, 200);
    ofPopMatrix();

    auto x = 600;
    auto offset = 105;
    
    // information text
    ofSetColor(255);
    ofDrawBitmapString("Angle", x, 20);

    for(int i =0; i < 3; i++){
        ofDrawBitmapString(threadedmmc.mmc.angle[i], x, (i + 1) * 50);
    }
    
    x+= offset;
        
    ofSetColor(255);
    ofDrawBitmapString("Accel", x, 20);

    for(int i =0; i < 3; i++){
        ofDrawBitmapString(threadedmmc.mmc.outputAcceleration[i], x, (i + 1) * 50);
    }
    
    x+= offset;
        
    ofSetColor(255);
    ofDrawBitmapString("Gyro", x, 20);

    for(int i =0; i < 3; i++){
        ofDrawBitmapString(threadedmmc.mmc.outputGyro[i], x, (i + 1) * 50);
    }
    
    x+= offset;
    
    ofSetColor(255);
    ofDrawBitmapString("Mag", x, 20);

    for(int i =0; i < 3; i++){
        ofDrawBitmapString(threadedmmc.mmc.outputMag[i], x, (i + 1) * 50);
    }

    auto threadFrame = threadedmmc.getThreadFrameNum();
    ofSetColor(255, 0, 0);
    ofDrawBitmapString("app frame: " + ofToString(ofGetFrameNum()), 20, 20);
    ofDrawBitmapString("thread frame: " + ofToString(threadFrame), 20, 35);
    ofDrawBitmapString("diff: " + ofToString(int64_t(ofGetFrameNum()) - threadFrame), 20, 50);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
    //reset orientation
    threadedmmc.mmc.recenter();
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{
}
