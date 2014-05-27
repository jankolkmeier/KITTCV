#include <opencv2/opencv.hpp>
#include <string>
#include "RemoteControl.h"
#include "Settings.h"

using namespace std;
using namespace cv;

int port = 9988;

// Settings
bool _flip = true;
double _scale = 0.2;
Settings * settings;
const char * settingsFile = "default.yml";


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

string paramDebugSize(int action, string val) {
    if (action == PARAM_GET) {
        ostringstream buf;
        buf << ctrl->width << " " << ctrl->height << " " << ctrl->elemSize;
        return buf.str();
    }
    return "";
}

string paramSave(int action, string val) {
    if (action == PARAM_SET) {
        if (val == "") {
            settings->save(settingsFile);
        } else {
            settings->save(val);
        }
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

    settings = new Settings();
    ctrl = new RemoteControl(port, settings);
    cap = new VideoCapture(0);
    settings->add("flip", &paramFlip, true);
    settings->add("scale", &paramScale, true);

    settings->add("SIZE", &paramDebugSize, false);
    settings->add("SAVE", &paramSave, false); // I
    settings->add("IMG", NULL, false);       // I
    settings->add("PARAMS", NULL, false);   // I

    if (!cap->isOpened()) {
        cout << "cant open capture device" << endl;
        return -1;
    }

    cap->read(frame);

    settings->save(settingsFile); // Save optional to instantiate file once
    settings->load(settingsFile); // Load Needed to set reduced through paramScale!

    cout << "Capturing: " << frame.cols << "x" << frame.rows << endl;
    cout << "Sending: " << reduced.cols << "x" << reduced.rows << endl;

    int key;
    while (key != 'q') {
        if (!cap->read(frame)) {
            cout << "Failed to read from capture device" << endl;
            ctrl->die("Stopped", 0);
            return -1;
        }

        pthread_mutex_lock(&(ctrl->mutex));

        reduceImage(frame, reduced, _scale);
        ctrl->is_data_ready = 1;

        pthread_mutex_unlock(&(ctrl->mutex));
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
