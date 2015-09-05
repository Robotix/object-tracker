#include <stdio.h>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <conio.h>
#include "tserial.h"

//comm.startDevice("COM2", 9600);
//comm.send_data(data);
//comm.stopDevice();

int color_threshold = 30;
int location_threshold = 40;
int angle_threshold = 2;
int h1 = 103;//204
int s1 = 185;//194
int v1 = 52;//166

int h2 = 126;//204
int s2 = 255;//194
int v2 = 255;//166

Tserial *com;

using namespace cv;
using namespace std;

Mat src; Mat src_gray;
int thresh = 100;
int max_thresh = 255;
RNG rng(12345);

void thresh_callback(int, void*);

/** @function main */
int main(int argc, char** argv)
{
	/// Load source image and convert it to gray
	com = new Tserial();
	if (com != 0)
	{
		com->connect("COM7", 9600, spNONE);
	}
	VideoCapture cap(2);
	namedWindow("Trackbars");
	createTrackbar("h1", "Trackbars", &h1, 255);
	createTrackbar("s1", "Trackbars", &s1, 255);
	createTrackbar("v1", "Trackbars", &v1, 255);
	createTrackbar("h2", "Trackbars", &h2, 255);
	createTrackbar("s2", "Trackbars", &s2, 255);
	createTrackbar("v2", "Trackbars", &v2, 255);
	while (1) {
		cap >> src;
		Mat imageHsv;
		Mat finalImg;
		cvtColor(src, imageHsv, CV_BGR2HSV);

		inRange(imageHsv, Scalar(h1,s1,v1), Scalar(h2,v2,s2), src_gray);

		//cvtColor(finalImg, src, CV_HSV2BGR);
		imshow("ragna", src_gray);

		

		/// Convert image to gray and blur it
		//cvtColor(src, src_gray, CV_BGR2GRAY);
		blur(src_gray, src_gray, Size(3, 3));

		/// Create Window
		char* source_window = "Source";
		namedWindow(source_window, CV_WINDOW_AUTOSIZE);
		imshow(source_window, src);

		createTrackbar(" Threshold:", "Source", &thresh, max_thresh, thresh_callback);
		thresh_callback(0, 0);

		waitKey(27);
	}
	return(0);
}

/** @function thresh_callback */
void thresh_callback(int, void*)
{
	Mat threshold_output;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	/// Detect edges using Threshold
	threshold(src_gray, threshold_output, thresh, 255, THRESH_BINARY);
	/// Find contours
	findContours(threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	/// Approximate contours to polygons + get bounding rects and circles
	vector<vector<Point> > contours_poly(contours.size());
	vector<Rect> boundRect;

	float area = INT_MAX;
	for (int i = 0; i < contours.size(); i++)
	{
		if (contourArea(contours[i]) > 10000 && contourArea(contours[i]) < 70000) {
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect.push_back(boundingRect(Mat(contours_poly[i])));
			area = contourArea(contours[i]);
		}
	}
	if (boundRect.size() == 0)
	{
		com->sendChar('g');
	}

	/// Draw polygonal contour + bonding rects + circles
	Mat drawing = Mat::zeros(threshold_output.size(), CV_8UC3);
	Point center_rect;
	for (int i = 0; i< boundRect.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
		center_rect = (boundRect[i].tl() + boundRect[i].br());
		center_rect.x /= 2;
		center_rect.y /= 2;
		circle(drawing, center_rect, 10, Scalar(255, 0, 0), CV_FILLED, 8, 0);
	}

	//logic
	int img_x = src.cols / 2;
	int obj_x = center_rect.x;
	if (img_x-100 > obj_x && 0 < obj_x) {
		cout << "left jao" << endl;
		com->sendChar('a');
	}
	else if (img_x + 100 < obj_x){
		cout << "right jao" << endl;
		com->sendChar('d');
	}
	else {
		if (area < 30000)
		{
			cout << "straight" << endl;
			com->sendChar('w');
		}		
		else
		{
			com->sendChar('g');
		}
	}

	/// Show in a window
	namedWindow("Contours", CV_WINDOW_AUTOSIZE);
	imshow("Contours", drawing);
	waitKey(100);
}
