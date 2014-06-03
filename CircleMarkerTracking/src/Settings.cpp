#include "Settings.h"
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

Settings::Settings() {
    nextId = 'a'; // we reserve 0-9
}

bool Settings::save(string name) {
    FileStorage fss(name, FileStorage::WRITE);

    for (paramMap::iterator it = params.begin(); it != params.end(); it++) {
        if (!it->second.doLoadSave) continue;
        string val = (*(it->second.callback))(PARAM_GET, "");
        fss << it->first << val;
    }

    fss.release();
    return true;
}


bool Settings::load(string name) {
    FileStorage fsl(name, FileStorage::READ);
    for (paramMap::iterator it = params.begin(); it != params.end(); it++) {
        if (!it->second.doLoadSave) continue;
        (*((paramCallback)(it->second.callback)))(PARAM_SET, (string)fsl[it->first]);
    }
    fsl.release();
    return true;
}


string Settings::get(string param) {
    return (*(params.at(param).callback))(PARAM_GET, "");
}

char Settings::paramId(string param) {
    return params.at(param).paramId;
}

void Settings::set(string param, string val) {
    (*(params.at(param).callback))(PARAM_SET, val);
}

void Settings::add(string k, paramCallback _c, bool _s, char id) {
    params.insert(pair<string, SettingsParam>(k, SettingsParam(_c, _s, id)));
}

void Settings::add(string k, paramCallback _c, bool _s) {
    params.insert(pair<string, SettingsParam>(k, SettingsParam(_c, _s, nextId)));
    nextId++;
}

string Settings::query() {
    ostringstream buf;

    for (paramMap::iterator it = params.begin(); it != params.end(); it++) {
        buf << it->second.paramId << ":" << it->first << endl;
    }

    return buf.str();
}

