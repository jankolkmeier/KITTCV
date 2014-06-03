#include <iostream>
#include <string>
#include <map>

using namespace std;
#define PARAM_SET 0
#define PARAM_GET 1

//typedef char *
typedef string (*paramCallback)(int, string);

struct SettingsParam {
    paramCallback callback;
    bool doLoadSave;
    char paramId;

    SettingsParam(){}

    SettingsParam(paramCallback _c, bool _dls, char _pid) {
        callback = _c;
        doLoadSave = _dls;
        paramId = _pid;
    }
};

typedef map<string, SettingsParam> paramMap;

class Settings {
    char nextId;
public:
    Settings();
    paramMap params;
    bool save(string);
    bool load(string);
    void add(string, paramCallback, bool);
    void add(string, paramCallback, bool, char);
    void set(string, string);
    string get(string);
    string query();
    char paramId(string);
};
