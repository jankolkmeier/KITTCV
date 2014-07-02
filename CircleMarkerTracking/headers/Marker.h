#include <opencv2/opencv.hpp>
#include <string>

class Marker {
public:
    Marker();
    cv::Mat R; // 3x3 Rotation
    cv::Mat t; // 3x1 Position
    cv::Mat T; // 4x4 Homog. Translation
    std::string serialize();
    void calculateTransform(cv::Mat rvec, cv::Mat tvec);
};
