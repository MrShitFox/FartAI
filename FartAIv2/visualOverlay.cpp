#include "visualOverlay.h"
#include <vector>
#include <mutex>
using namespace std;
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif
// Drawing parameters
static int g_boxBorderThickness = 1;
static int g_boxCornerRadius = 12;
static COLORREF g_boxColor = RGB(255, 0, 0);
static int g_fovBorderThickness = 3;
static COLORREF g_fovColor = RGB(255, 0, 0);
static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    vector<RECT>* monitors = reinterpret_cast<vector<RECT>*>(dwData);
    monitors->push_back(*lprcMonitor);
    return TRUE;
}
RECT GetCenteredRectOnMonitor(int monitorIndex, int w, int h) {
    vector<RECT> monitors;
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);
    if (monitors.empty() || monitorIndex >= monitors.size())
        monitorIndex = 0;
    RECT monitorRect = monitors[monitorIndex];
    int screenWidth = monitorRect.right - monitorRect.left;
    int screenHeight = monitorRect.bottom - monitorRect.top;
    int x = monitorRect.left + (screenWidth - w) / 2;
    int y = monitorRect.top + (screenHeight - h) / 2;
    return { x, y, x + w, y + h };
}
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int currentMonitor = 0;
    static int currentFov = 320;
    switch (uMsg) {
    case WM_CREATE: {
        SetTimer(hwnd, 1, 16, NULL);
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        // Initialization of initial values
        {
            scoped_lock lock(neuralFovMutex, monitorNumMutex);
            currentFov = config.neuralFov;
            currentMonitor = config.monitorNum;
        }
        return 0;
    }
    case WM_TIMER: {
        InvalidateRect(hwnd, NULL, FALSE);
        // Verify configuration changes
        int newMonitor, newFov;
        {
            scoped_lock lock(neuralFovMutex, monitorNumMutex);
            newMonitor = config.monitorNum;
            newFov = config.neuralFov;
        }
        // Updating the position when the monitor changes
        if (newMonitor != currentMonitor) {
            currentMonitor = newMonitor;
            RECT rc = GetCenteredRectOnMonitor(currentMonitor, 320, 320);
            SetWindowPos(hwnd, NULL, rc.left, rc.top, 320, 320,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        // Updating the FOV at a glance
        currentFov = newFov;
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
        // Background rendering
        HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rect, hBlackBrush);
        DeleteObject(hBlackBrush);
        // Drawing of detections
        vector<YoloResults> local_boxes;
        {
            lock_guard<mutex> lock(globalResultsMutex);
            local_boxes = globalResults;
        }
        for (auto& box : local_boxes) {
            int x1 = (int)box.box.x;
            int y1 = (int)box.box.y;
            int x2 = x1 + (int)box.box.width;
            int y2 = y1 + (int)box.box.height;
            if (x2 > x1 && y2 > y1) {
                HPEN pen = CreatePen(PS_SOLID, g_boxBorderThickness, g_boxColor);
                HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                HPEN oldPn = (HPEN)SelectObject(hdc, pen);
                RoundRect(hdc, x1, y1, x2, y2, g_boxCornerRadius, g_boxCornerRadius);
                SelectObject(hdc, oldPn);
                SelectObject(hdc, oldBr);
                DeleteObject(pen);
            }
        }
        // FOV rendering
        if (currentFov > 0) {
            int side = min(currentFov, 320);
            int cx = (rect.right - rect.left) / 2;
            int cy = (rect.bottom - rect.top) / 2;
            int half = side / 2;
            HPEN penFov = CreatePen(PS_SOLID, g_fovBorderThickness, g_fovColor);
            HBRUSH oldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            HPEN oldPn = (HPEN)SelectObject(hdc, penFov);
            Rectangle(hdc, cx - half, cy - half, cx + half, cy + half);
            SelectObject(hdc, oldPn);
            SelectObject(hdc, oldBr);
            DeleteObject(penFov);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY: {
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
void visualOverlay() {
    const wchar_t* CLASS_NAME = L"VisualOverlayClass";
    WNDCLASSEXW wc = { sizeof(WNDCLASSEX) };
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wc);
    while (true) {
        int initialMonitor, initialFov;
        {
            scoped_lock lock(neuralFovMutex, monitorNumMutex);
            initialMonitor = config.monitorNum;
            initialFov = config.neuralFov;
        }
        RECT rc = GetCenteredRectOnMonitor(initialMonitor, 320, 320);
        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            CLASS_NAME,
            L"VisualOverlay",
            WS_POPUP,
            rc.left, rc.top, 320, 320,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );
        if (hwnd) {
            SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
            ShowWindow(hwnd, SW_SHOW);
            MSG msg;
            while (GetMessage(&msg, NULL, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            DestroyWindow(hwnd);
        }
        else {
            break;
        }
    }
}