#include "overlay.h"

using namespace std;

extern wstring dynamicText;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    vector<RECT>* monitors = reinterpret_cast<vector<RECT>*>(dwData);
    monitors->push_back(*lprcMonitor);
    return TRUE;
}

void overlay() {
    const wchar_t CLASS_NAME[] = L"OverlayWindow";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        switch (uMsg) {
        case WM_CREATE: {
            SetTimer(hwnd, 1, 16, NULL);
            // Save the initial monitor number
            LPCREATESTRUCT createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            int initialMonitor = static_cast<int>(reinterpret_cast<ULONG_PTR>(createStruct->lpCreateParams));
            SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(initialMonitor));
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);

            HFONT hFont = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

            HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 0, 0));

            TextOutW(hdc, 10, 10, L"FartAI++", 8);
            TextOutW(hdc, 10, 40, dynamicText.c_str(), dynamicText.size());

            SelectObject(hdc, oldFont);
            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_TIMER: {
            InvalidateRect(hwnd, NULL, TRUE);

            // Updating the window position when changing the monitor
            vector<RECT> monitors;
            EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);

            int currentMonitor = static_cast<int>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            int newMonitor;

            {
                lock_guard<mutex> lock(monitorNumMutex);
                newMonitor = config.monitorNum;
            }

            // Monitor number validation
            if (newMonitor < 0 || newMonitor >= monitors.size()) {
                newMonitor = 0;
                {
                    lock_guard<mutex> lock(monitorNumMutex);
                    config.monitorNum = newMonitor;
                }
            }

            if (newMonitor != currentMonitor && !monitors.empty()) {
                RECT newRect = monitors[newMonitor];
                SetWindowPos(
                    hwnd,
                    HWND_TOPMOST,
                    newRect.left,
                    newRect.top,
                    newRect.right - newRect.left,
                    newRect.bottom - newRect.top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
                SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(newMonitor));
            }
            return 0;
        }
        case WM_DESTROY: {
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
        }
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };

    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = NULL;
    RegisterClassW(&wc);

    while (true) {
        vector<RECT> monitors;
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&monitors);

        int monitorNumLocal;
        {
            lock_guard<mutex> lock(monitorNumMutex);
            monitorNumLocal = config.monitorNum;
        }

        if (monitorNumLocal < 0 || monitorNumLocal >= monitors.size()) {
            monitorNumLocal = 0;
            {
                lock_guard<mutex> lock(monitorNumMutex);
                config.monitorNum = monitorNumLocal;
            }
        }

        RECT monitorRect = monitors[monitorNumLocal];

        HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            CLASS_NAME,
            L"Overlay",
            WS_POPUP,
            monitorRect.left,
            monitorRect.top,
            monitorRect.right - monitorRect.left,
            monitorRect.bottom - monitorRect.top,
            NULL,
            NULL,
            GetModuleHandle(nullptr),
            reinterpret_cast<LPVOID>(static_cast<ULONG_PTR>(monitorNumLocal))
        );

        if (!hwnd) return;

        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);
        ShowWindow(hwnd, SW_SHOW);

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Re-create the window on exiting the message loop
        DestroyWindow(hwnd);
    }
}