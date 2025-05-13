#ifndef CUSTOMWINDOWCAPTURE_H
#define CUSTOMWINDOWCAPTURE_H

#include <windows.h>
#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;

class CustomWindowCapture {
private:
    HWND hwndGame;
    HDC hdcGame, hDest;
    HBITMAP hbGame;
    BITMAPINFOHEADER bi;
    int offsetX, offsetY;
    int captureWidth, captureHeight;
    cv::Mat img;
    bool isInitialized; // Flag of successful initialization

public:
    CustomWindowCapture(const wchar_t* windowName, int fovSize);
    cv::Mat getFovFrame();
    bool isValid() const; // Method to verify successful initialization
    ~CustomWindowCapture();
};

bool isActive(const string& windowTitle);

#endif // CUSTOMWINDOWCAPTURE_H
