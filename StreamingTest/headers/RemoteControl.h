#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include <string>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>

class Settings;

class RemoteControl {
    static void *listen(void *arg);
public:
    bool running;
    RemoteControl(int port, Settings * s);
    void setDebugImage(cv::Mat &img);
    void die(std::string s, int c);

    pthread_t thread;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    struct sockaddr_in si_me;
    struct sockaddr_in si_other;
    socklen_t slen;
    int listenSock, is_data_ready;

    Settings * settings;

    cv::Mat sendFrame;
    int width;
    int height;
    int elemSize;
    int imgSize;
};
