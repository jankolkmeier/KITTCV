// OpenCVTest3.cpp : Defines the entry point for the console application.
//

#include "Config.h"
#include "MainTask.h"
#include "TestCircleMarkerDetectionTask.h"
#include "TrackingTestTask.h"

CVTask * task;

int main(int argc, char* argv[]) {

    task = new TrackingTestTask(0);

    while (!task->error) {
        task->loop();

        if (waitKey(10) == 'q')
            break;

    }

    return 0;
}
