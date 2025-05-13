#include "config.h"
using namespace std;

Config config;

vector<YoloResults> globalResults;
mutex globalResultsMutex;

wstring dynamicText = L"Status: Overlay started";
mutex dynamicTextMutex;

vector<TargetCords> globalTarget;
mutex globalTargetMutex;

cv::Mat global_image;
mutex global_image_mutex;

mutex aimMutex;
mutex aimAssistMutex;
mutex neuralFovMutex;
mutex userSmoothMutex;
mutex modelMutex;
mutex shootModeMutex;
mutex smartGenMutex;
mutex monitorNumMutex;

bool totalPizda = false;
mutex totalPizdaMutex;