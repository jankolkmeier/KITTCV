#include "MainTask.h"

double g_rho;      int rho_slider       = 1;
double g_theta;    int theta_slider     = 1;
double g_thresh;   int thresh_slider    = 4;
double g_min_line; int min_line_slider  = 35;
double g_line_gap; int line_gap_slider  = 10;

double g_a; int a_slider = 120;
double g_b; int b_slider = 40;
double g_c; int c_slider = 5;
double g_d; int d_slider = 127;
double g_e; int e_slider = 127;
double g_f; int f_slider = 500;

const int g_slider_max = 255;



MainTask::MainTask(int some_param) : CVTask() {
    FileStorage fs("calibration_jan.xml", FileStorage::READ);
    if (fs.isOpened()) {
        fs["Camera_Matrix"] >> cameraMatrix;
        fs["Distortion_Coefficients"] >> distCoeffs;
    } else {
        cout << "Cannot open calibration file" << endl;
        error = true;
        return;
    }
    fs.release();


    //namedWindow("Output1", CV_WINDOW_AUTOSIZE );
    //namedWindow("Output2", CV_WINDOW_AUTOSIZE );
    //namedWindow("ROI", CV_WINDOW_NORMAL );


    cap = new VideoCapture(0);
	if(!cap->isOpened()) {
        cout << "Cannot open capture device" << endl;
        error = true;
        return;
    }

    //cap->read(output);
    //ctrl->settings->load(settingsFile); // Load Needed to set reduced through paramScale!
	//cap.set(CV_CAP_PROP_FRAME_WIDTH,480); // calibration stuff 
    //cap.set(CV_CAP_PROP_FRAME_HEIGHT,320);
}

void MainTask::loop() {
    Mat img_capture;
    Mat img_undistort;
    //Mat img_crop;
	Mat imgL;
    Mat imgBw;

    cap->read(img_capture);
    undistort(img_capture, img_undistort, cameraMatrix, distCoeffs);
    //int bx = 20;
    //int by = 20;
    //Rect crop(bx, by, img_undistort.size().width-bx*2, img_undistort.size().height-by*2);
    //img_undistort(crop).copyTo(img_crop);
        
    Mat img_scene;
	cvtColor(img_undistort, img_scene, CV_BGR2GRAY);
	//blur(img_scene, img_scene, Size(2, 2));
	//img_scene.convertTo(imgL, -1, 100/g_d, 0);
	Canny(img_scene, img_bw, 80, 240, 3);
	vector<Point2f> good_corners;
    goodFeaturesToTrack(img_scene, good_corners, 10, 0.5, 10.0, Mat(), 10, false);
        
	//imshow("Output1", img_crop);


	vector<Vec4i> lines;
	vector<Vec4i> merged_lines;

	HoughLinesP(img_bw, lines, g_rho, g_theta, g_thresh, g_min_line, g_line_gap);

	vector<Point2i> corner_candidates;
    Mat img_show(img_bw.size(), CV_8UC3);
    cvtColor(img_bw, img_show, CV_GRAY2RGB); // for rendering output

	// Draw All Lines
	for (int i = 0; i < lines.size(); i++) {
		Vec4i v = lines[i];
		line(img_show, Point(v[0], v[1]), Point(v[2], v[3]), CV_RGB(0,255,0));
	}
		
    // Merge Lines
	for (int i = 0; i < lines.size(); i++) {
		Point2f p11 = Point2f(lines[i][0], lines[i][1]);
		Point2f p12 = Point2f(lines[i][2], lines[i][3]);
		for (int j = i+1; j < lines.size(); j++) {
			Point2f p21 = Point2f(lines[j][0], lines[j][1]);
			Point2f p22 = Point2f(lines[j][2], lines[j][3]);

			Point2f p00 = computeIntersect(lines[i], lines[j]);
                
			float p11_d = norm(p00-p11);
			float p12_d = norm(p00-p12);
			float p21_d = norm(p00-p21);
			float p22_d = norm(p00-p22);
                
            Vec4f dists( norm(p11-p21), norm(p11-p22), norm(p12-p21), norm(p12-p22) );
            int minidx = -1;
            double minval = 0.0f;
            minMaxIdx(dists, &minval, NULL, &minidx, NULL);

            double alpha = atan2(p12.y - p11.y, p12.x - p11.x) * 180.0 / CV_PI; 
			double beta = atan2(p22.y - p21.y, p22.x - p21.x) * 180.0 / CV_PI; 
			double ang = abs(alpha - beta);
			if (ang > 360) ang -= 360;
                
            if ((ang < 90+g_b && ang > 90-g_b) || (ang < 270+g_b && ang > 270-g_b)) {
				//if (p11_d > g_a && p12_d > g_a && p21_d > g_a && p22_d > g_a) {
                    
                if (minval < g_a) {
                    for (int g = 0; g < good_corners.size(); g++) {
                        if (norm(good_corners[g]-p00) < g_c) {
                            corner_candidates.push_back(p00);
                        }
				    }
                }
            }
		}
	}

        
        
    for (int i = 0; i < good_corners.size(); i++) {
	    circle(img_show, good_corners[i], 3, CV_RGB(0,255,255), 2);
    }

	// Draw corners
    for (int i = 0; i < corner_candidates.size(); i++) {
        //int c = classifyCorner(corner_candidates[i], img_scene);
        int c = 0;
        if (c == 1) {
			circle(img_show, corner_candidates[i], 3, CV_RGB(255,0,0), 2);
            break;
        } else {
			circle(img_show, corner_candidates[i], 3, CV_RGB(255,255,255), 2);
        }
	}
		
	//cout << "Lines: " << lines.size() << " Corners: " << corners.size() << endl;
	//imshow("Output2", img_show);
}



int classifyCorner(Point2i corner, Mat img) {
    int s = 100;
    if (corner.x <= s || corner.y <= s || corner.x >= img.size().width-s || corner.y >= img.size().height-s)
        return 0;

    Rect roi_r(corner.x-s/2, corner.y-s/2, s, s);
    Mat roi(img, roi_r);

    //imshow("ROI", roi);
    
    
    return 1;
}


Point2f computeIntersect(Vec4i a, Vec4i b) {
    int x1 = a[0], y1 = a[1], x2 = a[2], y2 = a[3];
    int x3 = b[0], y3 = b[1], x4 = b[2], y4 = b[3];

    if (float d = ((float)(x1-x2) * (y3-y4)) - ((y1-y2) * (x3-x4))) {
        Point2f pt;
        pt.x = ((x1*y2 - y1*x2) * (x3-x4) - (x1-x2) * (x3*y4 - y3*x4)) / d;
        pt.y = ((x1*y2 - y1*x2) * (y3-y4) - (y1-y2) * (x3*y4 - y3*x4)) / d;
        return pt;
    } else {
        return Point2f(-1, -1);
	}
}

double angle( Point pt1, Point pt2, Point pt0 ) {
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

void find_squares(Mat& image, vector<vector<Point> >& squares) {
    Mat blurred(image);
    medianBlur(image, blurred, 9);
    Mat gray0(blurred.size(), CV_8U), gray;
    vector<vector<Point> > contours;

    for (int c = 0; c < 3; c++) {
        int ch[] = {c, 0};
        mixChannels(&blurred, 1, &gray0, 1, ch, 1);
        const int threshold_level = 2;
        for (int l = 0; l < threshold_level; l++) {
            if (l == 0) {
                Canny(gray0, gray, 10, 20, 3);
                dilate(gray, gray, Mat(), Point(-1,-1));
            } else {
                gray = gray0 >= (l+1) * 255 / threshold_level;
            }

            findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
            vector<Point> approx;
            for (size_t i = 0; i < contours.size(); i++) {
                approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);
                if (approx.size() == 4 &&
                    fabs(contourArea(Mat(approx))) > 1000 &&
                    isContourConvex(Mat(approx))) {
                    double maxCosine = 0;

                    for (int j = 2; j < 5; j++) {
                        double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }

                    if (maxCosine < 0.3) {
                        squares.push_back(approx);
                    }
                }
            }
        }
    }
}

