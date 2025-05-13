//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#include <iostream>
#include <thread>
#include <windows.h>
#include <vector>
#include "aimLogic.h"
#include "aimRender.h"
#include "processing.h"
#include "overlay.h"
#include "visualOverlay.h"
#include "capture.h"
#include "gui.h"
#include "config.h"

using namespace std;

int main() {
    vector<HANDLE> handles;

    thread gui_thread(gui);
    thread capture_thread(capture);
    thread processing_thread(processing);
    thread aimLogic_thread(aimLogic);
    thread aimRender_thread(aimRender);
    //thread overlay_thread(overlay);
    //thread visual_overlay_thread(visualOverlay);

    // Собираем хэндлы всех потоков
    handles.push_back(gui_thread.native_handle());
    handles.push_back(capture_thread.native_handle());
    handles.push_back(processing_thread.native_handle());
    handles.push_back(aimLogic_thread.native_handle());
    handles.push_back(aimRender_thread.native_handle());
    //handles.push_back(overlay_thread.native_handle());
    //handles.push_back(visual_overlay_thread.native_handle());

    while (true) {
        if (totalPizda) {
            // Kill all threads
            for (HANDLE h : handles) {
                TerminateThread(h, 0);
            }

            // Detach all thread objects
            gui_thread.detach();
            capture_thread.detach();
            processing_thread.detach();
            aimLogic_thread.detach();
            aimRender_thread.detach();
            //overlay_thread.detach();
            //visual_overlay_thread.detach();

            return 0; // Immediate exit
        }

        this_thread::sleep_for(100ms);
    }

    // If totalPizda didn't work (shouldn't get here)
    gui_thread.join();
    capture_thread.join();
    processing_thread.join();
    aimLogic_thread.join();
    aimRender_thread.join();
    //overlay_thread.join();
    //visual_overlay_thread.join();

    return 0;
}