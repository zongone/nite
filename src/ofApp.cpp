#include "ofApp.h"

const int depthModeNo = 4;
const int colorModeNo = 15;
const int calibrationY = 35;

//--------------------------------------------------------------
void ofApp::setup()
{
	ofEnableAlphaBlending();
	ofSetFrameRate(30);
	ofSetWindowShape(ofGetScreenWidth(), ofGetScreenHeight());

	status = openni::OpenNI::initialize();
	if (status != openni::Status::STATUS_OK)
	{
		ofLogError() << "Initialize failed\n" << openni::OpenNI::getExtendedError();
		return;
	}

	openni::Array<openni::DeviceInfo> deviceInfoList;
	openni::OpenNI::enumerateDevices(&deviceInfoList);
	for (int i = 0; i<deviceInfoList.getSize(); i++)
	{
		ofLogNotice() << deviceInfoList.getSize();
	}

	status = device.open(openni::ANY_DEVICE);
	if (status != openni::Status::STATUS_OK)
	{
		ofLogError() << "Couldn't open device\n" << openni::OpenNI::getExtendedError();
		return;
	}

	device.setImageRegistrationMode(openni::ImageRegistrationMode::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	status = depthStream.create(device, openni::SensorType::SENSOR_DEPTH);
	if (status != openni::Status::STATUS_OK)
	{
		ofLogError() << "Couldn't create depth stream\n" << openni::OpenNI::getExtendedError();
	}

	const openni::Array<openni::VideoMode>& depthModes = depthStream.getSensorInfo().getSupportedVideoModes();

	openni::VideoMode depthMode = depthStream.getVideoMode();
	std::cout << "\nDefault depth mode: "
		<< depthMode.getResolutionX() << "x"
		<< depthMode.getResolutionY() << ", "
		<< depthMode.getFps() << ", "
		<< depthMode.getPixelFormat() << std::endl;

	std::cout << "Depth modes avaliable:\n";
	for (int i = 0; i < depthModes.getSize(); i++)
	{
		std::cout << i << ": " 
			<< depthModes[i].getResolutionX() << "x" 
			<< depthModes[i].getResolutionY() << ", " 
			<< depthModes[i].getFps() << " fps, " 
			<< depthModes[i].getPixelFormat() << " format\n";
	}

	depthStream.setVideoMode(depthModes[depthModeNo]);
	depthMode = depthStream.getVideoMode();
	std::cout << "Current depth mode: " 
		<< depthMode.getResolutionX() << "x" 
		<< depthMode.getResolutionY() << ", " 
		<< depthMode.getFps() << ", " 
		<< depthMode.getPixelFormat() << std::endl;

	depthResolutionX = depthMode.getResolutionX();
	depthResolutionY = depthMode.getResolutionY();

	status = colorStream.create(device, openni::SensorType::SENSOR_COLOR);
	if (status != openni::Status::STATUS_OK)
	{
		ofLogError() << "Couldn't create color stream\n" << openni::OpenNI::getExtendedError();
	}

	openni::VideoMode colorMode = colorStream.getVideoMode();
	std::cout << "\nDefault color mode: " 
		<< colorMode.getResolutionX() << "x" 
		<< colorMode.getResolutionY() << ", " 
		<< colorMode.getFps() << ", " 
		<< colorMode.getPixelFormat() << std::endl;

	const openni::Array<openni::VideoMode>& colorModes = colorStream.getSensorInfo().getSupportedVideoModes();
	std::cout << "Color modes avaliable:\n";
	for (int i = 0; i < colorModes.getSize(); i++)
	{
		std::cout << i << ": " 
			<< colorModes[i].getResolutionX() << "x" 
			<< colorModes[i].getResolutionY() << ", " 
			<< colorModes[i].getFps() << " fps, " 
			<< colorModes[i].getPixelFormat() << " format\n";
	}	
	colorStream.setVideoMode(colorModes[colorModeNo]);
	colorMode = colorStream.getVideoMode();
	std::cout << "Current video mode: "
		<< colorMode.getResolutionX() << "x" 
		<< colorMode.getResolutionY() << ", " 
		<< colorMode.getFps() << ", " 
		<< colorMode.getPixelFormat() << std::endl;

	colorResolutionX = colorMode.getResolutionX();
	colorResolutionY = colorMode.getResolutionY();

	status = colorStream.start();
	if (status != openni::Status::STATUS_OK)
	{
		ofLogError() << "Couldn't start color stream\n" << openni::OpenNI::getExtendedError();
	}

	niteStatus = nite::NiTE::initialize();
	if (niteStatus != nite::Status::STATUS_OK) {
		ofLogError() << "Couldn't start NiTE";
		return;
	}

	niteStatus = userTracker.create();
	if (niteStatus != nite::Status::STATUS_OK) {
		ofLogError() << "Couldn't create user tracker";
		return;
	}

//	depthPixels.allocate(0, 0, 1);
	//depthTexture.clear();
}

//--------------------------------------------------------------
void ofApp::update(){
	
	status = colorStream.readFrame(&colorFrame);
	const openni::RGB888Pixel* colorPixelsHandler = (openni::RGB888Pixel*)colorFrame.getData();

	colorPixels.setFromPixels((const unsigned char*)colorPixelsHandler, colorResolutionX, colorResolutionY, ofImageType::OF_IMAGE_COLOR);
	colorPixels.crop(0, calibrationY, colorResolutionX, colorResolutionX/resolutionRatio);
	colorTexture.loadData(colorPixels);

	// get raw depth frame
	niteStatus = userTracker.readFrame(&userTrackerFrame);

	// get user labels map

	const nite::UserId* pixelLabels = userTrackerFrame.getUserMap().getPixels();

	// process users
	const nite::Array<nite::UserData>& usersData = userTrackerFrame.getUsers();

	for (int i = 0; i < usersData.getSize(); ++i) {
		const nite::UserData& user = usersData[i];

		int id = user.getId();

		if (user.isNew()) {
			userTracker.startSkeletonTracking(id);
		}
		else if (user.getSkeleton().getState() == nite::SkeletonState::SKELETON_TRACKED && id < MAX_USERS - 1) {
			users[id].visible = user.isVisible();
		
			users[id].head = getJointInDepthCoordinates(user, nite::JOINT_HEAD);
			users[id].neck = getJointInDepthCoordinates(user, nite::JOINT_NECK);
			users[id].leftShoulder = getJointInDepthCoordinates(user, nite::JOINT_LEFT_SHOULDER);
			users[id].rightShoulder = getJointInDepthCoordinates(user, nite::JOINT_RIGHT_SHOULDER);
			users[id].leftElbow = getJointInDepthCoordinates(user, nite::JOINT_LEFT_ELBOW);
			users[id].rightElbow = getJointInDepthCoordinates(user, nite::JOINT_RIGHT_ELBOW);
			users[id].leftHand = getJointInDepthCoordinates(user, nite::JOINT_LEFT_HAND);
			users[id].rightHand = getJointInDepthCoordinates(user, nite::JOINT_RIGHT_HAND);
			users[id].torso = getJointInDepthCoordinates(user, nite::JOINT_TORSO);
			users[id].leftHip = getJointInDepthCoordinates(user, nite::JOINT_LEFT_HIP);
			users[id].rightHip = getJointInDepthCoordinates(user, nite::JOINT_RIGHT_HIP);
			users[id].leftKnee = getJointInDepthCoordinates(user, nite::JOINT_LEFT_KNEE);
			users[id].rightKnee = getJointInDepthCoordinates(user, nite::JOINT_RIGHT_KNEE);
			users[id].leftFoot = getJointInDepthCoordinates(user, nite::JOINT_LEFT_FOOT);
			users[id].rightFoot = getJointInDepthCoordinates(user, nite::JOINT_RIGHT_FOOT);
		}
	}

	// get depth frame data
	openni::VideoFrameRef frame = userTrackerFrame.getDepthFrame();
	calculateHistogram(depthHist, MAX_DEPTH, frame);

	const openni::DepthPixel* depthRow = (const openni::DepthPixel*)frame.getData();

	depthPixels.allocate(depthResolutionX, depthResolutionY, ofImageType::OF_IMAGE_COLOR_ALPHA);

	for (int y = 0; y < depthResolutionY; y++) {
		for (int x = 0; x < depthResolutionX; x++, depthRow++, pixelLabels++) {
			float histogramValue = depthHist[*depthRow];
	
			// filter out everything that's not a user
			if (*pixelLabels == 0) {
				depthPixels.setColor(x, y, ofColor(255,255,255,0));
			}
			else {
				depthPixels.setColor(x, y, ofColor(histogramValue/**ofRandom(1),value*ofRandom(1),value*ofRandom(1))*/));
			}
		}
	}

	depthPixels.crop(0,0,depthResolutionX, depthResolutionX/resolutionRatio);

	if (!depthTexture.isAllocated() || depthTexture.getWidth() != depthResolutionX || depthTexture.getHeight() != depthResolutionY) {
		depthTexture.allocate(depthPixels);
	}
	depthTexture.loadData(depthPixels);
}

//--------------------------------------------------------------
void ofApp::draw(){
	
	ofDrawBitmapString(ofToString(ofGetFrameRate()),10,10);
	/*ofBackground(0,255,0);
	background.draw(0, 0);*/

	ofSetColor(255, 255, 255);
	colorTexture.draw(0, 0,targetResolutionX,targetResolutionY);

	if (depthTexture.isAllocated()) {
		depthTexture.draw(0, 0,targetResolutionX, targetResolutionY);
	}

	ofSetColor(ofColor(255, 0, 0));
	for (int i = 0; i < MAX_USERS; ++i) {
		if (users[i].visible) {
			drawUser(users[i]);
		}
	}
	ofDrawBitmapString(ofToString(ofGetFrameRate()),10,10);
}

void ofApp::calculateHistogram(float* histogram, int histogramSize, const openni::VideoFrameRef& frame)
{
	const openni::DepthPixel* depth = (const openni::DepthPixel*)frame.getData();

	memset(histogram, 0, histogramSize * sizeof(float));

	int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel) - frame.getWidth();
	int height = frame.getHeight();
	int width = frame.getWidth();


	unsigned int pointsNum = 0;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x, ++depth) {
			if (*depth != 0) {
				histogram[*depth]++;
				pointsNum++;
			}
		}
		depth += restOfRow;
	}

	for (int i = 1; i < histogramSize; i++) {
		histogram[i] += histogram[i - 1];
	}

	if (pointsNum) {
		for (int i = 1; i < histogramSize; i++) {
			histogram[i] = (256 * (1.0f - (histogram[i] / pointsNum)));
		}
	}
}

ofVec2f ofApp::getJointInDepthCoordinates(nite::UserData user, nite::JointType jointType)
{
	const nite::SkeletonJoint& joint = user.getSkeleton().getJoint(jointType);
	float x, y;

	userTracker.convertJointCoordinatesToDepth(joint.getPosition().x, joint.getPosition().y, joint.getPosition().z, &x, &y);
	double resizeRatio = targetResolutionX / depthResolutionX;
	return ofVec2f(x*resizeRatio, y*resizeRatio);
}

void ofApp::drawUser(user_t user)
{
	float r = 3;

	double resizeFactor = targetResolutionX / depthResolutionX;

	ofDrawCircle(user.head, r);
	ofDrawCircle(user.neck, r);
	ofDrawCircle(user.leftShoulder, r);
	ofDrawCircle(user.rightShoulder, r);
	ofDrawCircle(user.leftElbow, r);
	ofDrawCircle(user.rightElbow, r);
	ofDrawCircle(user.leftHand, r);
	ofDrawCircle(user.rightHand, r);
	ofDrawCircle(user.torso, r);
	ofDrawCircle(user.leftHip, r);
	ofDrawCircle(user.rightHip, r);
	ofDrawCircle(user.leftKnee, r);
	ofDrawCircle(user.rightKnee, r);
	ofDrawCircle(user.leftFoot, r);
	ofDrawCircle(user.rightFoot, r);

	ofDrawLine(user.head, user.neck);
	ofDrawLine(user.leftShoulder, user.rightShoulder);
	ofDrawLine(user.leftShoulder, user.torso);
	ofDrawLine(user.rightShoulder, user.torso);

	ofDrawLine(user.leftShoulder, user.leftElbow);
	ofDrawLine(user.leftElbow, user.leftHand);

	ofDrawLine(user.rightShoulder, user.rightElbow);
	ofDrawLine(user.rightElbow, user.rightHand);

	ofDrawLine(user.torso, user.leftHip);
	ofDrawLine(user.torso, user.rightHip);

	ofDrawLine(user.leftHip, user.leftKnee);
	ofDrawLine(user.leftKnee, user.leftFoot);

	ofDrawLine(user.rightHip, user.rightKnee);
	ofDrawLine(user.rightKnee, user.rightFoot);
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

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
