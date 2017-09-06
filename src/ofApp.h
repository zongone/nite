#pragma once

#include "ofMain.h"
#include "NiTE.h"

#define MAX_USERS 10
#define MAX_DEPTH 10000

struct user_t {
	bool visible;
	ofVec2f head;
	ofVec2f neck;
	ofVec2f leftShoulder;
	ofVec2f rightShoulder;
	ofVec2f leftElbow;
	ofVec2f rightElbow;
	ofVec2f leftHand;
	ofVec2f rightHand;
	ofVec2f torso;
	ofVec2f leftHip;
	ofVec2f rightHip;
	ofVec2f leftKnee;
	ofVec2f rightKnee;
	ofVec2f leftFoot;
	ofVec2f rightFoot;
	ofVec3f centerOfMassWorldCoord;
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void calculateHistogram(float* histogram, int histogramSize, const openni::VideoFrameRef& frame);
		ofVec2f getJointInDepthCoordinates(nite::UserData user, nite::JointType jointType);
		void drawUser(user_t user);

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

private:
	openni::Device device;
	openni::VideoStream depthStream;
	openni::VideoStream colorStream;

	int colorResolutionX;
	int colorResolutionY;
	int depthResolutionX;
	int depthResolutionY;

	int targetResolutionX = 1920;
	int targetResolutionY = 1080;

	double resolutionRatio = (double)targetResolutionX / targetResolutionY;

	openni::Status status;
	nite::Status niteStatus;

	openni::VideoFrameRef colorFrame;

	nite::UserTracker userTracker;
	nite::UserTrackerFrameRef userTrackerFrame;

	ofPixels depthPixels;
	ofPixels colorPixels;
	ofTexture depthTexture;
	ofTexture colorTexture;
	
	float depthHist[MAX_DEPTH];
	user_t users[MAX_USERS];
};
