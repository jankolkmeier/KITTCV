#include "TestCircleMarkerDetectionTask.h"

TestCircleMarkerDetectionTask::TestCircleMarkerDetectionTask(int some_param) : CVTask() {
    cap = new VideoCapture(0);
}

void TestCircleMarkerDetectionTask::loop() {
    Mat input;// = src;
	(*cap) >> input;
    Mat output = input.clone();
    vector<Point3i> circles;
    Mat gray, bw;
	cvtColor(input, gray, CV_BGR2GRAY);
    //blur(gray, gray, Size(3, 3));
    //adaptiveThreshold(gray, bw, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 100, 3);
    threshold(gray, bw, 128, 255, THRESH_BINARY);

    //Mat row = bw.col(296);
    imshow("BW", bw);

    search(bw, circles);
    //findNestedCirlces(bw, circles);
    
    for (int i = 0; i < circles.size(); i++) {
        Point center(circles[i].x, circles[i].y);
        circle(output, center, circles[i].z, CV_RGB(0,255,255), 2);
    }

    imshow("NestedCircles", output);
}
