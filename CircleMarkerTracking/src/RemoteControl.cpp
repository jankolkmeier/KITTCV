#include "RemoteControl.h"
#include "Settings.h"

using namespace std;
using namespace cv;

RemoteControl::RemoteControl(int port, string _settingsFile) {
    settingsFile = _settingsFile;
    settings = new Settings();
    settings->add("PARAMS",   NULL, false, '0');
    settings->add("SAVE",     NULL, false, '1');
    settings->add("IMG",      NULL, false, '2');
    settings->add("SIZE",     NULL, false, '3');

    running = true;
    slen = sizeof(si_other);
    if ((listenSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("socket", 1);
    }

    image_requested = 0;

    memset((char *) &si_me, 0, sizeof(si_me));

    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenSock, (struct sockaddr*)&si_me, sizeof(si_me)) == -1) {
        die("bind", 1);
    }

    if (pthread_create(&thread, NULL, RemoteControl::listen, this)) {
        die("Failed to create thread.", 1);
    }
}

void RemoteControl::setDebugImage(cv::Mat &img) {
    width = img.cols;
    height = img.rows;
    elemSize = img.elemSize();
    imgSize = img.total()*elemSize;
    sendFrame = (img.reshape(0,1));
}

// Listens for commands in seperate threads and responds accordingly
void * RemoteControl::listen(void * parent) {
    RemoteControl * self = ((RemoteControl *) parent);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    char pbuf[128];
    size_t recv_len = 0;

    /* Listen on UDP port */
    cout << "Waiting for requests..." << endl;
    while(self->running) {
        fflush(stdout);

        // Wait for Request...
        if ((recv_len = recvfrom(self->listenSock, pbuf, 128, 0, (struct sockaddr *) &(self->si_other), &(self->slen))) == -1) {
            self->die("recvfrom()", 1);
        }

        pbuf[recv_len] = '\0';
        string req(pbuf);

        cout << "Handling '" << req << "' (" << req.length() << ")" << endl;
        // TODO:
        //   - toggle debug console stuff
        //   - Make library (and use with cmake in other project)

        // Handle Request...
        if (req.compare(0, 3, "GET") == 0) {
            // GET_param
            string param = req.substr(4);
            char prefix = self->settings->paramId(param);

            if (param == "PARAMS") { // Query available parameters
                // Special: query
                string query = self->settings->query();
                query = prefix + query;
                if (sendto(self->listenSock, query.c_str(), query.length(), 0, (struct sockaddr*) &(self->si_other), self->slen) == -1) {
                    self->die("params sendto()", 1);
                }
            } else if (param == "IMG") {
                // Special: image
                self->image_requested = 1;
                while (self->image_requested != 2) {
                    usleep(1);
                }

                pthread_mutex_lock(&(self->mutex));
                Mat out;
                Mat prefixMat(1, 1, self->sendFrame.type());
                prefixMat.at<int>(0,0) = prefix;
                hconcat(prefixMat, self->sendFrame, out);
                // data should be smaller than 254x254 (65535 bytes) according to UDP spec?
                if (sendto(self->listenSock, out.data, self->imgSize+1, 0, (struct sockaddr*) &(self->si_other), self->slen) == -1) {
                    self->die("image sendto()", 1);
                }
                self->image_requested = 0;
                pthread_mutex_unlock(&(self->mutex));
            } else if (param == "SIZE") {
                ostringstream _res; 
                _res << prefix << self->width << " " << self->height << " " << self->elemSize;
                string res = _res.str();
                if (sendto(self->listenSock, res.c_str(), res.length(), 0, (struct sockaddr*) &(self->si_other), self->slen) == -1) {
                    self->die("size sendto()", 1);
                }
            } else {
                // Regular: get value
                string val = self->settings->get(param);
                val = prefix + val;
                if (sendto(self->listenSock, val.c_str(), val.length(), 0, (struct sockaddr*) &(self->si_other), self->slen) == -1) {
                    self->die("request sendto()", 1);
                }
            }
        } else if (req.compare(0, 3, "SET") == 0) { // Set parameter value
            // SET_param[_value]
            int paramNameEnd = req.find(' ', 4);
            bool no_val = (paramNameEnd == -1) || (paramNameEnd == req.length()-1);
            string param = req.substr(4, paramNameEnd-4);
            string val = no_val ? "" : req.substr(paramNameEnd+1);
            if (param == "SAVE") {
                if (no_val) {
                    self->settings->save(self->settingsFile);
                } else {
                    self->settings->save(val);
                }
            } else {
                self->settings->set(param, val);
            }
        }
        pthread_testcancel();
    }
}

void RemoteControl::die(std::string s, int retval) {
    if (retval == 0) {
        std::cout << "Die: " << s << std::endl;
    } else {
        std::cerr << s << std::endl;
    }


    if (listenSock) {
        close(listenSock);
    }

    pthread_mutex_destroy(&mutex);
    exit(retval);
}
