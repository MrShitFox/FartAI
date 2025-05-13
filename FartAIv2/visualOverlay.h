#pragma once
#include "iostream"
#include "Windows.h"
#include "mutex"
#include "yoloLib.h"
#include "config.h"
#include "vector"
#include "CustomWindowCapture.h"
#pragma comment(lib, "msimg32.lib")

static RECT GetCenteredRectOnPrimaryMonitor(int w, int h);

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void visualOverlay();