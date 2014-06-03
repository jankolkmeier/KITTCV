#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

void  search(Mat & img, vector<Point3i> & circles);
bool _search(Mat & row, int r, vector<Point3i> & bars, bool horizontal);