#pragma once
#include "iostream"
#include "yoloLib.h"
#include "CustomWindowCapture.h"
#include "config.h"
#include "filesystem"
#include "Windows.h"
#include "mutex"
#include <sstream>
#include <chrono>
#include "DesktopDuplicationCapture.h"

using namespace std;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

void processing();