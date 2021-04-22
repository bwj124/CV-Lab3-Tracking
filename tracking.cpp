#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int smin = 15;
int vmin = 40;
int vmax = 256;
int bins = 16;
int main(int argc, char** argv) {
	VideoCapture capture;
	capture.open(0);
	if (!capture.isOpened()) {
		printf("could not find video data file...\n");
		return -1;
	}
	namedWindow("MeanShift/CAMShift Tracking", WINDOW_AUTOSIZE);
	namedWindow("ROI Histogram", WINDOW_AUTOSIZE);

	vector<Point> pt;
	bool firstRead = true;
	float hrange[] = { 0, 180 };
	const float* hranges = hrange;
	Rect selection;
	Mat frame, hsv, hue, mask, hist, backprojection;
	Mat drawImg = Mat::zeros(300, 300, CV_8UC3);
	while (capture.read(frame)) {
		if (firstRead) {
			Rect2d first = selectROI("CAMShift Tracking", frame);
			selection.x = first.x;
			selection.y = first.y;
			selection.width = first.width;
			selection.height = first.height;
			printf("ROI.x= %d, ROI.y= %d, width = %d, height= %d", selection.x, selection.y, selection.width, selection.height);
		}
		// convert to HSV
		cvtColor(frame, hsv, COLOR_BGR2HSV);
		inRange(hsv, Scalar(0, smin, vmin), Scalar(180, vmax, vmax), mask);
		hue = Mat(hsv.size(), hsv.depth());
		int channels[] = { 0, 0 };
		mixChannels(&hsv, 1, &hue, 1, channels, 1);

		if (firstRead) {
			// ROI ֱ��ͼ����
			Mat roi(hue, selection);
			Mat maskroi(mask, selection);
			calcHist(&roi, 1, 0, maskroi, hist, 1, &bins, &hranges);
			normalize(hist, hist, 0, 255, NORM_MINMAX);

			// show histogram
			int binw = drawImg.cols / bins;
			Mat colorIndex = Mat(1, bins, CV_8UC3);
			for (int i = 0; i < bins; i++) {
				colorIndex.at<Vec3b>(0, i) = Vec3b(saturate_cast<uchar>(i * 180 / bins), 255, 255);
			}
			cvtColor(colorIndex, colorIndex, COLOR_HSV2BGR);
			for (int i = 0; i < bins; i++) {
				int  val = saturate_cast<int>(hist.at<float>(i) * drawImg.rows / 255);
				rectangle(drawImg, Point(i * binw, drawImg.rows), Point((i + 1) * binw, drawImg.rows - val), Scalar(colorIndex.at<Vec3b>(0, i)), -1, 8, 0);
			}
		}

		// back projection
		calcBackProject(&hue, 1, 0, hist, backprojection, &hranges);
		// CAMShift tracking
		backprojection &= mask;
		//ʹ��MeanShift�㷨
		//int num = meanShift(backprojection, selection, TermCriteria((TermCriteria::COUNT | TermCriteria::EPS), 10, 1));
		//Point2f center = Point2f(selection.x + selection.width / 2, selection.y + selection.height / 2);
		//RotatedRect trackBox = RotatedRect(center, Point2f(selection.width, selection.height), 30);

		//ʹ��CamShift�㷨
		RotatedRect trackBox = CamShift(backprojection, selection, TermCriteria((TermCriteria::COUNT | TermCriteria::EPS), 10, 1));
		Point2f center = Point2f(selection.x + selection.width / 2, selection.y + selection.height / 2);

		// draw location on frame;
		ellipse(frame, trackBox, Scalar(0, 0, 255), 3, 8);
		pt.push_back(center);

		for (int i = 0; i < pt.size() - 1; i++)
		{
			line(frame, pt[i], pt[i + 1], Scalar(0, 255, 0), 2.5); //��������
		}
		if (firstRead) {
			firstRead = false;
		}
		imshow("MeanShift/CAMShift Tracking", frame);
		imshow("ROI Histogram", drawImg);
		char c = waitKey(50);// ESC
		if (c == 27) {
			break;
		}
	}

	capture.release();
	waitKey(0);
	return 0;
}

