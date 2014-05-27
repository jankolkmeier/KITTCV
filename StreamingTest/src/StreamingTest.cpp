#include <opencv2/opencv.hpp>
#include <string>
#include "RemoteControl.h"
#include "Settings.h"

using namespace std;
using namespace cv;

int port = 9988;
const char * settingsFile = "default.yml";

// Settings
bool _flip = true;
double _scale = 0.2;

Mat frame, reduced;
Mat shadow;
VideoCapture * cap;
RemoteControl * ctrl;

void reduceImage(Mat &src, Mat &dst, float scale);

string paramScale(int action, string val) {
    if (action == PARAM_SET) {
        _scale = max(0.05, atof(val.c_str()));
        reduceImage(frame, reduced, _scale);
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

int main(int argc, char** argv) {
    if (argc == 1) {
        cout << "Using default port (" << port << ") and settings file (";
        cout <<  settingsFile << ")" << endl;
    } else if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc == 3) {
        port = atoi(argv[1]);
        settingsFile = argv[2];
    } else {
        cout << "Usage: " << argv[0] << " [port] [settingsFile]" << endl;
        return -1;
    }

    ctrl = new RemoteControl(port, settingsFile);
    cap = new VideoCapture(0);
    cap->set(CV_CAP_PROP_FRAME_WIDTH, 320);
    cap->set(CV_CAP_PROP_FRAME_HEIGHT, 240);

    ctrl->settings->add("flip", &paramFlip, true);
    ctrl->settings->add("scale", &paramScale, true);

    if (!cap->isOpened()) {
        cout << "cant open capture device" << endl;
        return -1;
    }

    cap->read(frame);

    //settings->save(settingsFile); // Save optional to instantiate file once
    ctrl->settings->load(settingsFile); // Load Needed to set reduced through paramScale!

    cout << "Capturing: " << frame.cols << "x" << frame.rows << endl;
    cout << "Sending: " << reduced.cols << "x" << reduced.rows << endl;

    int key;
    while (key != 'q') {
        if (!cap->read(frame)) {
            cout << "Failed to read from capture device" << endl;
            ctrl->die("Stopped", 0);
            return -1;
        }
        
        if (ctrl->image_requested == 1) {
            reduceImage(frame, reduced, _scale);
        }

        if (ctrl->image_requested == 1) {
            pthread_mutex_lock(&(ctrl->mutex));
            ctrl->image_requested = 2;
            pthread_mutex_unlock(&(ctrl->mutex));
        }

        key = waitKey(10);
    }

    ctrl->die("Stopped", 0);
    return 0;
}

void reduceImage(Mat &src, Mat &dst, float scale) {
    Mat tmp;
    cvtColor(src, tmp, CV_RGB2GRAY);
    if (_flip)
        flip(tmp, tmp, 1);
    resize(tmp, dst, Size(), scale, scale, INTER_NEAREST);
}
