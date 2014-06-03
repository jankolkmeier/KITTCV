#include "CVTask.h"
#include "CircleMarkerDetection.h"

class TrackingTestTask: public CVTask {
public:
    TrackingTestTask(int some_param);
    void loop();

private:
    double a;
    vector<Point3d> object3;
            
    vector<Point3f> cosx; vector<Point2f> cosx_img;
    vector<Point3f> cosy; vector<Point2f> cosy_img;
    vector<Point3f> cosz; vector<Point2f> cosz_img;

    VideoCapture * cap;
    Mat cameraMatrix;
    Mat distCoeffs;
    Mat rvec, tvec;
    Mat defDistCoeffs;

    Mat src;
};
