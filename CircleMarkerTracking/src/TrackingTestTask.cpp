#include "TrackingTestTask.h"
#include <string>
#include "RemoteControl.h"
#include "Settings.h"
#include "Config.h"
#include "Profiling.h"

const int framesAvg = 5;
int frameCount;
int tGrabbing[framesAvg];
int tUndistort[framesAvg];
int tThreshold[framesAvg];
int tSearch[framesAvg];
int tPost[framesAvg];
int tCtrl[framesAvg];
int tTotal[framesAvg];

RemoteControl * ctrl;

int port = 9988;
const char * settingsFile = "default.yml";

// Settings
bool _flip = true;
double _scale = 0.2;

Mat output, reduced;
Mat shadow;

void reduceImage(Mat &src, Mat &dst, float scale);
string paramScale(int action, string val);
string paramFlip(int action, string val);
string paramProfile(int action, string val);

TrackingTestTask::TrackingTestTask(int some_param) : CVTask() {
    frameCount = 0;
    if (true) {
        cout << "Using default port (" << port << ") and settings file (";
        cout <<  settingsFile << ")" << endl;
    } /*else if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc == 3) {
        port = atoi(argv[1]);
        settingsFile = argv[2];
    } else {
        cout << "Usage: " << argv[0] << " [port] [settingsFile]" << endl;
        return -1;
    }*/

    ctrl = new RemoteControl(port, settingsFile);

    ctrl->settings->add("flip", &paramFlip, true);
    ctrl->settings->add("scale", &paramScale, true);
    ctrl->settings->add("profile", &paramProfile, false);

    //src = imread("circletest4.png");
    //src = imread("resources/test/posetest_fail_1.png");
    #if CircleMarkerTracking_GUI == 1
        namedWindow("Output", CV_WINDOW_AUTOSIZE );
        namedWindow("ROI", CV_WINDOW_NORMAL );
        namedWindow("BW", CV_WINDOW_AUTOSIZE );
    #endif

    //FileStorage fs("calibration_jan.xml", FileStorage::READ);
    FileStorage fs("resources/calibration/phone.xml", FileStorage::READ);
    if (fs.isOpened()) {
        fs["camera_matrix"] >> cameraMatrix;
        fs["distortion_coefficients"] >> distCoeffs;
        cout << "C: " << cameraMatrix << endl;
        cout << "D: " << distCoeffs << endl;
    } else {
        cout << "Cannot open calibration file" << endl;
        error = true;
        return;
    }
    fs.release();

    //cap = new VideoCapture("resources/test/test210502.mp4");
    //cap = new VideoCapture("resources/test/test210501.mp4");
    cap = new VideoCapture(0);
    if (!cap->isOpened()) {
        error = true;
        cout << "Cannot open Capture Device" << endl;
        return;
    }
    cap->set(CV_CAP_PROP_FRAME_WIDTH, 320);
    cap->set(CV_CAP_PROP_FRAME_HEIGHT, 240);

    Mat tmp;
    cap->read(tmp);
    //undistort(tmp, output, cameraMatrix, distCoeffs);
    resize(tmp, output, Size(0,0), 0.5, 0.5, 1);


    // todo: if file ! exists
    //ctrl->settings->save(settingsFile); // Save optional to instantiate file once
    ctrl->settings->load(settingsFile); // Load Needed to set reduced through paramScale!

    rvec = Mat(3,1,cv::DataType<double>::type);
    tvec = Mat(3,1,cv::DataType<double>::type);

    defDistCoeffs = Mat(4,1,cv::DataType<double>::type);
    defDistCoeffs.at<double>(0) = 0;
    defDistCoeffs.at<double>(1) = 0;
    defDistCoeffs.at<double>(2) = 0;
    defDistCoeffs.at<double>(3) = 0;

    a = 18.5 / 2.0;
    object3.push_back(Point3d(-a, a, 0));
    object3.push_back(Point3d( a, a, 0));
    object3.push_back(Point3d( a,-a, 0));
    object3.push_back(Point3d(-a,-a, 0));
    cosx.push_back(Point3f(0,0,0));
    cosx.push_back(Point3f(a,0,0));
    cosy.push_back(Point3f(0,0,0));
    cosy.push_back(Point3f(0,a,0));
    cosz.push_back(Point3f(0,0,0));
    cosz.push_back(Point3f(0,0,a));
}

void TrackingTestTask::loop() {
    Mat input, gray, bw;
    int64 t0;
    int64 ttotal = GetTimeMs64();

    if (frameCount == (framesAvg-1))
        frameCount = 0;
    else
        frameCount++; 

    t0 = GetTimeMs64();
    if (!cap->read(input)) {
        cap->release();
        cout << "No more img data" << endl;
        error = true;
        return;
    }
    tGrabbing[frameCount] = (int)(GetTimeMs64() - t0);
    //input = src;
    //output = frame;

    t0 = GetTimeMs64();
    //undistort(input, output, cameraMatrix, distCoeffs);
    resize(input, output, Size(0,0), 0.5, 0.5, 1);

    cvtColor(output, gray, CV_BGR2GRAY);
    Size size = gray.size();
    tUndistort[frameCount] = (int)(GetTimeMs64() - t0);

    t0 = GetTimeMs64();
    //blur(gray, gray, Size(3, 3));
    //adaptiveThreshold(gray, bw, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 15, 0);
    threshold(gray, bw, 75, 255, THRESH_BINARY);
    #if CircleMarkerTracking_GUI == 1
        imshow("BW", bw);
    #endif
    tThreshold[frameCount] = (int)(GetTimeMs64() - t0);

    t0 = GetTimeMs64();
    vector<Point3i> circles;
    //imshow("BW", bw.row(269));
    //_search(bw.row(269), 269, circles, true);
    search(bw, circles);
    tSearch[frameCount] = (int)(GetTimeMs64() - t0);

    t0 = GetTimeMs64();
    for (int i = 0; i < circles.size(); i++) {
        Point center(circles[i].x, circles[i].y);
        circle(output, center, circles[i].z, CV_RGB(0,255,255), 2);
        int range = circles[i].z*2.0;

        int x1 = max(0, circles[i].x-range);
        int y1 = max(0, circles[i].y-range);
        int x2 = min(size.width, circles[i].x+range);
        int y2 = min(size.height, circles[i].y+range);

        //cout << y1 << ":" << y2 << " " << x1 << ":" << x2 << " of " << size.height << "x" << size.width << endl;
        //Mat roi = gray(Range(y1, y2), Range(x1, x2));
        Mat roi = bw(Range(y1, y2), Range(x1, x2));
        roi = Mat::ones(roi.size(), roi.type()) * 255 - roi;
        #if CircleMarkerTracking_GUI == 1
            imshow("ROI", roi);
        #endif

        vector< vector<Point> > contours;
        vector<Vec4i> hierarchy;

        findContours(roi, contours, hierarchy,
                     CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(x1, y1));

        // iterate through all the top-level contours,
        // draw each connected component with its own random color
        int idx = 0;
        if (contours.size() > 0) {


            vector<Point2i> approx;
            approxPolyDP(contours[0], approx, 20, true);

            for (int c = 0; c < approx.size(); c++) {
                circle(output, approx[c], 5, Scalar(255,0,255));
            }

            if (approx.size() == 4) {
                vector<Point2d> scene; // Find corners in scene and "sort" them...
                scene.push_back(Point2d(approx[2].x, approx[2].y));
                scene.push_back(Point2d(approx[3].x, approx[3].y));
                scene.push_back(Point2d(approx[0].x, approx[0].y));
                scene.push_back(Point2d(approx[1].x, approx[1].y));

                solvePnP(object3, scene, cameraMatrix, defDistCoeffs, rvec, tvec, false, CV_ITERATIVE);// CV_P3P, CV_EPNP, CV_ITERATIVE



                Mat R;
                Rodrigues(rvec, R); // R is 3x3
                R = R.t();  // rotation of inverse
                Mat t = -R * tvec; // translation of inverse
                Mat T(4, 4, R.type()); // T is 4x4
                T(Range(0,3), Range(0,3)) = R * 1; // copies R into T
                T(Range(0,3), Range(3,4)) = t * 1; // copies tvec into T
                // fill the last row of T (NOTE: depending on your types, use float or double)
                double *p = T.ptr<double>(3);
                p[0] = p[1] = p[2] = 0;
                p[3] = 1;

                cout << t << endl;
                // T is a 4x4 matrix with the pose of the camera in the object frame





                std::vector<Point2d> scene_corners3(4);
                projectPoints(object3, rvec, tvec, cameraMatrix, defDistCoeffs, scene_corners3);
                projectPoints(cosx, rvec, tvec, cameraMatrix, distCoeffs, cosx_img);
                projectPoints(cosy, rvec, tvec, cameraMatrix, distCoeffs, cosy_img);
                projectPoints(cosz, rvec, tvec, cameraMatrix, distCoeffs, cosz_img);

                // Lines around marker
                line(output, scene_corners3[0], scene_corners3[1], Scalar(255, 0, 255), 1);
                line(output, scene_corners3[1], scene_corners3[2], Scalar(255, 0, 255), 1);
                line(output, scene_corners3[2], scene_corners3[3], Scalar(255, 0, 255), 1);
                line(output, scene_corners3[3], scene_corners3[0], Scalar(255, 0, 255), 1);

                // Marker COS
                line(output, cosx_img[0], cosx_img[1], Scalar(255,0,0), 2);
                line(output, cosy_img[0], cosy_img[1], Scalar(0,255,0), 2);
                line(output, cosz_img[0], cosz_img[1], Scalar(0,0,255), 2);
            }
        }
    }
    tPost[frameCount] = (int)(GetTimeMs64() - t0);

    t0 = GetTimeMs64();
    if (ctrl->image_requested == 1) {
        reduceImage(output, reduced, _scale);
    }

    if (ctrl->image_requested == 1) {
        pthread_mutex_lock(&(ctrl->mutex));
        ctrl->image_requested = 2;
        pthread_mutex_unlock(&(ctrl->mutex));
    }
    tCtrl[frameCount] = (int)(GetTimeMs64() - t0);

    #if CircleMarkerTracking_GUI == 1
        imshow("Output", output);
    #endif
    tTotal[frameCount] = (int)(GetTimeMs64() - ttotal);
}

void reduceImage(Mat &src, Mat &dst, float scale) {
    Mat tmp;
    cvtColor(src, tmp, CV_RGB2GRAY);
    if (_flip)
        flip(tmp, tmp, 1);
    resize(tmp, dst, Size(), scale, scale, INTER_NEAREST);
}

string paramScale(int action, string val) {
    if (action == PARAM_SET) {
        _scale = min(max(0.05, atof(val.c_str())), 1.0);
        reduceImage(output, reduced, _scale);
        ctrl->setDebugImage(reduced);
    } else {
        ostringstream buf;
        buf << _scale;
        return buf.str();
    }
    return "";
}

string paramFlip(int action, string val) {
    if (action == PARAM_SET) {
        _flip = atoi(val.c_str()) == 1;
    } else {
        return _flip ? "1" : "0";
    }
    return "";
}

int avgArray(int arr[]) {
    int sum = 0;
    for (int i = 0; i < framesAvg; i++) {
        sum += arr[i];
    }
    return sum / framesAvg;
}

string paramProfile(int action, string val) {
    if (action == PARAM_GET) {
        ostringstream buf;
        buf << "Grabbing:" << avgArray(tGrabbing) << "\n";
        buf << "Undistort:" << avgArray(tUndistort) << "\n";
        buf << "Threshold:" << avgArray(tThreshold) << "\n";
        buf << "Search:" << avgArray(tSearch) << "\n";
        buf << "Post:" << avgArray(tPost) << "\n";
        buf << "Ctrl:" << avgArray(tCtrl) << "\n";
        buf << "Total:" << avgArray(tTotal);
        return buf.str();
    }
    return "";
}

