#include "PoseEstimation.h"
#include <Windows.h>

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#include <stdio.h>
#include <iostream>

enum face_objects_t { NOSE, LEFTEYE, RIGHTEYE, MOUTH };

using namespace std;

static void draw_point(cv::Mat im, cv::Point2d point, Scalar color)
{ /* Temp function */
	circle(im, point, 3, color, -1);
}

cv::Point2d get_center_point(Rect face, Rect candidate)
{
	return cv::Point2d(face.x + candidate.x + candidate.width*0.5,
		face.y + candidate.y + candidate.height*0.5);
}

static void add_best_point(face_objects_t obj_as_idx, cv::Mat im, Rect face, Rect obj,
	vector<cv::Point2d> &image_points, Scalar color = Scalar(255, 0, 0))
{
	cv::Point2d center = get_center_point(face, obj);
	image_points[obj_as_idx] = center;
	draw_point(im, center, color);
}

#define N_POINTS 4

vector<cv::Point2d> image_points(N_POINTS);
static vector<bool> found_point(N_POINTS);

bool is_left_eye(Rect eye, Rect face)
{
	return eye.x + eye.width*0.5 < face.width*0.5;
}

void calcMatrix(Mat im);
unsigned int frameCount;

void detect2dpoints(Mat im)
{
	Mat frame_gray;
	Mat faceROI;
	Rect2d faceRect, noseRect;
	
	cvtColor(im, frame_gray, CV_BGR2GRAY);
	if (!faceDetector.try_detect(im, frame_gray, &faceRect, Size(im.rows / 5.0, im.rows / 5.0))) {
		cv::imshow("Output", im);
		return;
	}
	faceROI = frame_gray(faceRect);
	rectangle(im, faceRect, Scalar(255, 0, 255), 2);
	
	if (!noseDetector.try_detect(im, faceROI, &noseRect,
		Size(faceRect.width*0.15, faceRect.height*0.15), Size(faceRect.width*0.3, faceRect.height*0.3))) {
		cv::imshow("Output", im);
		return;
	}
	add_best_point(NOSE, im, faceRect, noseRect, image_points);
	
	if (!eyesDetector.try_detect_pair(im, faceROI, faceRect,
		Size(faceRect.width*0.15, faceRect.height*0.15), Size(faceRect.width*0.3, faceRect.height*0.3))) {
		cv::imshow("Output", im);
		return;
	}
	add_best_point(LEFTEYE, im, faceRect, eyesDetector.getLeftDetectedRect(), image_points, Scalar(255, 255, 0));
	add_best_point(RIGHTEYE, im, faceRect, eyesDetector.getRightDetectedRect(), image_points, Scalar(0, 255, 0));

	if (!mouthDetector.try_detect(im, faceROI, &mouthRect,
		Size(faceRect.width*0.15, faceRect.height*0.15), Size(faceRect.width*0.3, faceRect.height*0.3))) {
		cv::imshow("Output", im);
		return;
	}
	add_best_point(MOUTH, im, faceRect, mouthRect, image_points, Scalar(0, 255, 255));
	cv::imshow("Output", im);
	//calcMatrix(im);
}

void calcMatrix(Mat im) {
	// 3D model points.
	std::vector<cv::Point3d> model_points;
	std::vector<cv::Point2d> img_points_new;
	if (found_point[NOSE]) {
		model_points.push_back(Point3f(-0.0697709f, 18.6015f, 87.9695f));
		//model_points.push_back(cv::Point3d(0.0f, 0.0f, 0.0f));               // Nose tip
		img_points_new.push_back(image_points[NOSE]);
	}
	model_points.push_back(cv::Point3d(0.0f, -330.0f, -65.0f));          // Chin
	if (found_point[LEFTEYE]) {
		model_points.push_back(Point3f(-36.9522f, 39.3518f, 47.1217f));
		//model_points.push_back(cv::Point3d(-225.0f, 170.0f, -135.0f));       // Left eye left corner
		img_points_new.push_back(image_points[LEFTEYE]);
	}
	if (found_point[RIGHTEYE]) {
		model_points.push_back(Point3f(35.446f, 38.4345f, 47.6468f));
		//model_points.push_back(cv::Point3d(225.0f, 170.0f, -135.0f));        // Right eye right corner
		img_points_new.push_back(image_points[RIGHTEYE]);
	}
	if (found_point[MOUTH]) {
		model_points.push_back(Point3f(-0.0697709f, -29.2935f, 72.7329f));
		//model_points.push_back(cv::Point3d(0.0f, -150.0f, -125.0f));      // Mouth
		img_points_new.push_back(image_points[MOUTH]);
	}
	//model_points.push_back(cv::Point3d(-150.0f, -150.0f, -125.0f));      // Left Mouth corner
	//model_points.push_back(cv::Point3d(150.0f, -150.0f, -125.0f));       // Right mouth corner
	// Camera internals
	int count = 0;
	for (int i = 0; i < found_point.size(); i++) {
		count += found_point[i];
	}

	if (count < 4) {
		cv::imshow("Output", im);
		return;
	}
	cout << count;
	double focal_length = MAX(im.cols, im.rows); // Approximate focal length.
	Point2d center = cv::Point2d(im.cols / 2, im.rows / 2);
	cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_length, 0, center.x,
		0, focal_length, center.y,
		0, 0, 1);
	cv::Mat dist_coeffs = cv::Mat::zeros(4, 1, cv::DataType<double>::type); // Assuming no lens distortion

																			//cout << "Camera Matrix " << endl << camera_matrix << endl;
																			// Output rotation and translation
	cv::Mat rotation_vector; // Rotation in axis-angle form
	cv::Mat translation_vector;

	// Solve for pose
	if (cv::solvePnP(model_points, img_points_new, camera_matrix, dist_coeffs, rotation_vector, translation_vector, false, CV_ITERATIVE)) {
		cout << "ok!";
	} else {
		cv::imshow("Output", im);
		return;
	}

	// Project a 3D point (0, 0, 1000.0) onto the image plane.
	// We use this to draw a line sticking out of the nose

	vector<Point3d> nose_end_point3D;
	vector<Point2d> nose_end_point2D;

	double length = 500.0;
	nose_end_point3D.push_back(Point3d(length, 0, 0));
	nose_end_point3D.push_back(Point3d(-length, 0, 0));
	nose_end_point3D.push_back(Point3d(0, length, 0));
	nose_end_point3D.push_back(Point3d(0, -length, 0));
	nose_end_point3D.push_back(Point3d(0, 0, length));
	nose_end_point3D.push_back(Point3d(0, 0, -length));

	projectPoints(nose_end_point3D, rotation_vector, translation_vector, camera_matrix, dist_coeffs, nose_end_point2D);

	cv::line(im, image_points[NOSE], nose_end_point2D[0], cv::Scalar(255, 0, 0), 2);
	cv::line(im, image_points[NOSE], nose_end_point2D[2], cv::Scalar(0, 255, 0), 2);
	cv::line(im, image_points[NOSE], nose_end_point2D[4], cv::Scalar(0, 0, 255), 2); // z
	/*
	cv::line(im, nose_end_point2D[0], nose_end_point2D[1], cv::Scalar(255, 0, 0), 2);
	cv::line(im, nose_end_point2D[2], nose_end_point2D[3], cv::Scalar(0, 255, 0), 2);
	cv::line(im, nose_end_point2D[4], nose_end_point2D[5], cv::Scalar(0, 0, 255), 2); // z
	*/
	/*cout << "Rotation Vector " << endl << rotation_vector << endl;
	cout << "Translation Vector" << endl << translation_vector << endl;

	cout << nose_end_point2D << endl;*/

	// Display image.
	cv::imshow("Output", im);
}

