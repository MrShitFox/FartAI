#include "CustomWindowCapture.h"

CustomWindowCapture::CustomWindowCapture(const wchar_t* windowName, int fovSize) : isInitialized(false) {
    hwndGame = FindWindowW(NULL, windowName);
    if (!hwndGame) {
        cerr << "Window not found" << endl;
        return; // Just exit, the object remains uninitialized
    }

    hdcGame = GetDC(hwndGame);
    hDest = CreateCompatibleDC(hdcGame);

    RECT rect;
    GetClientRect(hwndGame, &rect);
    captureWidth = fovSize;
    captureHeight = fovSize;
    offsetX = (rect.right - captureWidth) / 2;
    offsetY = (rect.bottom - captureHeight) / 2;

    hbGame = CreateCompatibleBitmap(hdcGame, captureWidth, captureHeight);
    SelectObject(hDest, hbGame);

    bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = captureWidth;
    bi.biHeight = -captureHeight; // for image orientation
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    img = cv::Mat(captureHeight, captureWidth, CV_8UC4);

    isInitialized = true; // Setting a successful initialization
}

bool CustomWindowCapture::isValid() const {
    return isInitialized;
}

cv::Mat CustomWindowCapture::getFovFrame() {
    if (!IsWindow(hwndGame)) { // Check if the window exists
        //cerr << "Window is no longer valid" << endl;
        return cv::Mat(); // Return empty matrix
    }

    BitBlt(hDest, 0, 0, captureWidth, captureHeight, hdcGame, offsetX, offsetY, SRCCOPY);
    GetDIBits(hdcGame, hbGame, 0, captureHeight, img.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    return img;
}

CustomWindowCapture::~CustomWindowCapture() {
    if (isInitialized) {
        ReleaseDC(hwndGame, hdcGame);
        DeleteObject(hbGame);
        DeleteDC(hDest);
    }
}

bool isActive(const string& windowTitle) {
    // Get the handle of the active window
    HWND activeWindow = GetForegroundWindow();

    if (activeWindow == nullptr) {
        return false; // If there is no active window, return false
    }

    // Получаем длину заголовка окна
    int length = GetWindowTextLength(activeWindow);
    if (length == 0) {
        return false; // If the header is empty, return false
    }

    // Getting the title of the active window
    char* buffer = new char[length + 1];
    GetWindowText(activeWindow, buffer, length + 1);

    // Convert to std::string for comparison
    string activeTitle(buffer);
    delete[] buffer;

    // Comparing the title of the active window with the given title
    return activeTitle == windowTitle;
}