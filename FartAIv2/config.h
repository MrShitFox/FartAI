#pragma once
#include "yoloLib.h"

using namespace std;

struct Config {
    bool aim;
    bool aimAssist;
    int neuralFov;
    float userSmooth;
    int model;

    //0 - auto
    //1 - body
    //2 - head
    int shootMode;

    bool smartGen;
    int monitorNum;

    Config() : aim(true), aimAssist(false), neuralFov(320), userSmooth(0.350f), model(0), shootMode(2), smartGen(true), monitorNum(0) {}
};

extern Config config;

//-----------------
struct TargetCords {
    float x;
    float y;
};

extern vector<TargetCords> globalTarget;
extern mutex globalTargetMutex;

//-------------------


extern vector<YoloResults> globalResults;
extern mutex globalResultsMutex;

extern wstring dynamicText;
extern mutex dynamicTextMutex;

extern cv::Mat global_image;
extern mutex global_image_mutex;

extern mutex aimMutex;
extern mutex aimAssistMutex;
extern mutex neuralFovMutex;
extern mutex userSmoothMutex;
extern mutex modelMutex;
extern mutex shootModeMutex;
extern mutex smartGenMutex;
extern mutex monitorNumMutex;

extern bool totalPizda;
extern mutex totalPizdaMutex;