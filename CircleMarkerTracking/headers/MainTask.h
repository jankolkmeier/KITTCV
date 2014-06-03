#include "CVTask.h"
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/nonfree/features2d.hpp"

Point2f computeIntersect(Vec4i a, Vec4i b);
int classifyCorner(Point2i corner, Mat img);

class MainTask: public CVTask {
public:
    MainTask(int some_param);
    void loop();

private:
    VideoCapture * cap;
    Mat img_bw;
    
    Mat cameraMatrix;
    Mat distCoeffs;
};
