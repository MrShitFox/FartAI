#ifndef AIM_MODULES_H
#define AIM_MODULES_H

#include <vector>
#include <utility>
#include <cmath>
#include <random>
#include <opencv2/core/types.hpp>
#include "yoloLib.h"
#include <numeric>
#include <windows.h>
#include <algorithm>
#include <optional>
#include "config.h"

using namespace std;

struct Monitor {
    int top;
    int left;
    int width;
    int height;

    Monitor(int top, int left, int width, int height) : top(top), left(left), width(width), height(height) {}

    friend ostream& operator<<(ostream& os, const Monitor& m) {
        os << "Monitor {top: " << m.top << ", left: " << m.left << ", width: " << m.width << ", height: " << m.height << "}";
        return os;
    }
};

struct Enemy {
    YoloResults body;       // optional
    YoloResults head;       // optional
    bool hasBody = false;
    bool hasHead = false;
};

        
pair<int, int> get_screen_size();

vector<pair<float, float>> wind_mouse(float start_x, float start_y, float dest_x, float dest_y, double G_0 = 9, double W_0 = 3, double M_0 = 15, double D_0 = 12);

pair<float, float> screen_to_angle(int x, int y, int screen_width, int screen_height, float fov);

pair<float, float> compensate_coordinates(int x, int y, float distance, int screen_width, int screen_height, float fov);

pair<float, float> get_box_center(const cv::Rect_<float>& box);

optional<pair<float, float>> find_closest_target(const vector<YoloResults>& boxes, const Monitor& monitor, int shootMode);

void MoveCursor(int deltaX, int deltaY);

vector<Enemy> groupEnemies(const vector<YoloResults>& boxes);

pair<int, int> lerp(int startX, int startY, int endX, int endY, float t);
#endif
