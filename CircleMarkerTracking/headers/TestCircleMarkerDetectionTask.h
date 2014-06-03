#include "CVTask.h"
#include "CircleMarkerDetection.h"

class TestCircleMarkerDetectionTask: public CVTask {
public:
    TestCircleMarkerDetectionTask(int some_param);
    void loop();

private:
    Mat src;
    VideoCapture * cap;
};
