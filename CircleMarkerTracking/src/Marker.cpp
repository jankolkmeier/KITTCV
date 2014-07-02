#include "Marker.h"

using namespace cv;
using namespace std;

Marker::Marker() {
    T = Mat(4, 4, DataType<double>::type);
    R = Mat(3, 3, DataType<double>::type);
    double *p = T.ptr<double>(3);
    p[0] = p[1] = p[2] = 0; p[3] = 1;
}

void Marker::calculateTransform(Mat rvec, Mat tvec) {
    Rodrigues(rvec, R);
    R = R.t();  // rotation of inverse
    t = -R * tvec; // translation of inverse
    T(Range(0,3), Range(0,3)) = R * 1; // copies R into T
    T(Range(0,3), Range(3,4)) = t * 1; // copies tvec into T
    //cout << T << endl;

}


string Marker::serialize() {
    ostringstream buf;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (r+c > 0) buf << " ";
            buf << ((int) (T.at<double>(r, c) * 1000));
        }
    }
    return buf.str();
}
