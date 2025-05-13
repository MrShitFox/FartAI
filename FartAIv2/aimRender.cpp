#include "aimRender.h"
using namespace std;
static const auto FRAME_DURATION = std::chrono::milliseconds(8);
static double partialX = 0.0;
static double partialY = 0.0;
void aimRender() {
    bool aimLocal;
    bool aimAssistLocal;
    float userSmoothLocal;
    // Get the screen dimensions + calculate the center
    auto [screen_width, screen_height] = get_screen_size();
    int widthCenter = screen_width / 2;
    int heightCenter = screen_height / 2;
    // Storing the path and index of the current point
    vector<pair<float, float>> currentPath;
    size_t currentIndex = 0;
    auto lastFrameTime = chrono::steady_clock::now();
    while (true) {
        {
            scoped_lock lock(aimMutex, aimAssistMutex, userSmoothMutex);
            aimLocal = config.aim;
            aimAssistLocal = config.aimAssist;
            userSmoothLocal = config.userSmooth;
        }
        auto nextFrameTime = lastFrameTime + FRAME_DURATION;
        lastFrameTime = nextFrameTime;
        // Checking if Aim and AimAssist are enabled
        if (!aimLocal || (aimAssistLocal && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))) {
            currentPath.clear();
            currentIndex = 0;
            this_thread::sleep_until(nextFrameTime);
            continue;
        }
        // Obtaining the last target coordinates
        float targetX, targetY;
        {
            lock_guard<mutex> lock(globalTargetMutex);
            if (globalTarget.empty()) {
                currentPath.clear();
                currentIndex = 0;
                this_thread::sleep_until(nextFrameTime);
                continue;
            }
            targetX = globalTarget.back().x;
            targetY = globalTarget.back().y;
        }
        // Checking the availability of boxes (targets)
        {
            bool boxesEmpty = false;
            {
                lock_guard<mutex> lock(globalResultsMutex);
                boxesEmpty = globalResults.empty();
            }
            if (boxesEmpty) {
                currentPath.clear();
                currentIndex = 0;
                std::this_thread::sleep_until(nextFrameTime);
                continue;
            }
        }
        //cout << targetX << "  " << targetY << endl;
        // Generating mouse path to target coordinates
        currentPath = wind_mouse(
            widthCenter,
            heightCenter,
            static_cast<int>(targetX),
            static_cast<int>(targetY)
        );
        currentIndex = 0;
        if (currentPath.empty()) {
            this_thread::sleep_until(nextFrameTime);
            continue;
        }
        constexpr int STEPS_PER_FRAME = 5;
        static int prevAbsX = widthCenter;
        static int prevAbsY = heightCenter;
        for (int step = 0; step < STEPS_PER_FRAME; ++step) {
            if (currentIndex >= currentPath.size())
                break;
            if (!aimLocal || (aimAssistLocal && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000)))
                break;
            auto [absX, absY] = currentPath[currentIndex];
            if (currentIndex == 0) {
                // First time - reset so that there is no “jump” from (widthCenter, heightCenter)
                prevAbsX = widthCenter;
                prevAbsY = heightCenter;
            }
            // Calculation of base offsets
            double dxFloat = (absX - prevAbsX);
            double dyFloat = (absY - prevAbsY);
            // Apply sensitivity (smoothing)
            double sensitivity = userSmoothLocal;
            dxFloat *= sensitivity;
            dyFloat *= sensitivity;
            // “Accumulator” for floating values
            partialX += dxFloat;
            partialY += dyFloat;
            // Rounded to whole pixels
            int dx = static_cast<int>(floor(partialX + 0.5));
            int dy = static_cast<int>(floor(partialY + 0.5));
            // If there is a real offset (dx or dy != 0) - move the cursor
            if (dx != 0 || dy != 0) {
                MoveCursor(dx, dy);
                // Subtract the “used” part
                partialX -= dx;
                partialY -= dy;
            }
            // Remembering previous coordinates
            prevAbsX = absX;
            prevAbsY = absY;
            currentIndex++;
        }
        //cout << "---------" << endl;
        this_thread::sleep_until(nextFrameTime);
    }
}