#pragma once

#include "ofMain.h"
#include "metamotionController.h"
#include "ThreadedMMC.h"
#include "ofxOsc.h"

// send host (aka ip address)
#define HOST "localhost"

/// send port
#define PORT 12345

class ofApp : public ofBaseApp{
    void sendOSCMessages();

    public:
        void setup();
        void update();
        void draw();

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
    
        float frameRate;
        ThreadedMMC threadedmmc;
    
        ofxOscSender sender;
};
