#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/tracking.hpp>
#include <opencv/cxcore.h>

using namespace cv;

#define FACE_DETECTION_INTERVAL 100
#define NOSE_DETECTION_INTERVAL 1
#define MOUTH_DETECTION_INTERVAL 1
#define EYES_DETECTION_INTERVAL 1

#include <string>

class BaseDetector
{
public:
	BaseDetector();
	~BaseDetector();
	int load_cascade();
	virtual Rect2d getDetectedRect() const { return _detectedRect; }
	virtual void resetDetectedFlag(bool value);
protected:
	virtual Rect2d bestCandidate(std::vector<Rect> &candidates, Rect2d faceRect = Rect2d()) { return candidates[0]; }
	CascadeClassifier _cascade;
	String _cascadeName;
	Ptr<Tracker> _tracker;
	Rect2d _detectedRect;
	bool _isDetected;
	int _frame_count;
	unsigned int _interval;
	std::string _trackerName;
};

enum FaceDetectionStatus { TRACKING_STATUS, DETECT_STATUS, FAIL_STATUS };
class FaceDetector : public BaseDetector
{
public:
	FaceDetector();	
	~FaceDetector();
	bool try_detect(Mat im, Mat frame_gray, Size minSize = Size(), Size maxSize = Size());
	FaceDetectionStatus getStatus();
private:
	FaceDetectionStatus _status;
	bool detect_and_tracker(Mat im, Mat frame_gray, Size minSize = Size(), Size maxSize = Size());
};

class FaceElementsDetector : public BaseDetector
{
public:
	FaceElementsDetector();
	~FaceElementsDetector();
	virtual bool try_detect(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size());
protected:
	virtual bool detect_and_tracker(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size());
};

class NoseDetector : public FaceElementsDetector
{
public:
	NoseDetector();
	~NoseDetector();
	bool try_detect(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size()) override;
	Rect2d bestCandidate(std::vector<Rect> &candidates, Rect2d faceRect = Rect2d()) override;
private:
	bool detect_and_tracker(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size()) override;
};

class MouthDetector : public FaceElementsDetector
{
public:
	MouthDetector();
	~MouthDetector();
	bool try_detect(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size()) override;
private:
	Rect2d bestCandidate(std::vector<Rect> &candidates, Rect2d faceRect = Rect2d()) override;
	bool detect_and_tracker(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size()) override;
};

class PairFaceElementsDetector : public FaceElementsDetector
{
public:
	PairFaceElementsDetector();
	~PairFaceElementsDetector();
	virtual bool try_detect_pair(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size());
	Rect2d getLeftDetectedRect() { return _detectedLeftRect; }
	Rect2d getRightDetectedRect() { return _detectedRightRect; }
	void resetDetectedFlag(bool value) override;
private:
	bool is_left(Rect eye, Rect face);
	void add_point(cv::Mat im, Rect face, Rect eye);
protected:
	virtual bool detect_and_tracker(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size());
	Ptr<Tracker> _leftTracker;
	Ptr<Tracker> _rightTracker;
	Rect2d _detectedLeftRect;
	Rect2d _detectedRightRect;
	bool _isDetectedLeft;
	bool _isDetectedRight;
};

class EyesDetector : public PairFaceElementsDetector
{
public:
	EyesDetector();
	~EyesDetector();
	bool try_detect_pair(Mat im, Mat frame_gray, Rect2d faceRect, Size minSize = Size(), Size maxSize = Size()) override;
};

extern FaceDetector faceDetector;
extern NoseDetector noseDetector;
extern EyesDetector eyesDetector;
extern MouthDetector mouthDetector;