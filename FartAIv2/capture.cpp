#include "capture.h"
using namespace std;
void capture() {
    DesktopDuplicationCapture* capture = nullptr;
    int retryDelay = 1000;
    // Initialization of local variables with mutex protection
    int currentFov;
    int currentMonitor;
    {
        scoped_lock lock(neuralFovMutex, monitorNumMutex);
        currentFov = config.neuralFov;
        currentMonitor = config.monitorNum;
    }
    // Initialization of the capture
    while (true) {
        {
            scoped_lock lock(neuralFovMutex, monitorNumMutex);
            capture = new DesktopDuplicationCapture(config.monitorNum, config.neuralFov);
        }
        if (capture->isValid()) {
            {
                lock_guard<mutex> lock(dynamicTextMutex);
                dynamicText = L"Status: Monitor captured successfully";
            }
            break;
        }
        delete capture;
        {
            lock_guard<mutex> lock(dynamicTextMutex);
            dynamicText = L"Status: Capture initialization failed. Retrying...";
        }
        this_thread::sleep_for(chrono::milliseconds(retryDelay));
    }
    // Basic capture cycle
    while (true) {
        // Configuration update with locking
        int newFov, newMonitor;
        {
            scoped_lock lock(neuralFovMutex, monitorNumMutex);
            newFov = config.neuralFov;
            newMonitor = config.monitorNum;
        }
        // Checking parameter changes
        if (newFov != currentFov || newMonitor != currentMonitor) {
            delete capture;
            {
                scoped_lock lock(neuralFovMutex, monitorNumMutex);
                capture = new DesktopDuplicationCapture(config.monitorNum, config.neuralFov);
            }
            if (capture->isValid()) {
                currentFov = newFov;
                currentMonitor = newMonitor;
                {
                    lock_guard<mutex> lock(dynamicTextMutex);
                    dynamicText = L"Status: Capture reconfigured successfully";
                }
            }
            else {
                {
                    lock_guard<mutex> lock(dynamicTextMutex);
                    dynamicText = L"Status: Reconfiguration failed. Retrying...";
                }
                delete capture;
                continue;
            }
        }
        // Receiving a frame
        cv::Mat image = capture->getFovFrame();
        // Image loss processing
        if (image.empty()) {
            {
                lock_guard<mutex> lock(dynamicTextMutex);
                dynamicText = L"Status: Monitor signal lost. Reconnecting...";
            }
            // Reconnect cycle
            while (true) {
                delete capture;
                {
                    scoped_lock lock(neuralFovMutex, monitorNumMutex);
                    capture = new DesktopDuplicationCapture(config.monitorNum, config.neuralFov);
                }
                if (capture->isValid()) {
                    {
                        lock_guard<mutex> lock(dynamicTextMutex);
                        dynamicText = L"Status: Connection restored";
                    }
                    break;
                }
                delete capture;
                this_thread::sleep_for(chrono::milliseconds(retryDelay));
            }
            continue;
        }
        // Saving the result
        {
            lock_guard<mutex> img_lock(global_image_mutex);
            global_image = image.clone();
        }
    }
    // Cleanup (never reached in current flow)
    delete capture;
}