#ifndef CVTASK_H
#define CVTASK_H

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class CVTask {
    
    public:
        CVTask();
        virtual void loop();
        bool error;

};

#endif