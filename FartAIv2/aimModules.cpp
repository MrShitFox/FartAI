#include "aimModules.h"


pair<int, int> get_screen_size() {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    return { width, height };
}

vector<pair<float, float>> wind_mouse(float start_x, float start_y, float dest_x, float dest_y, double G_0, double W_0, double M_0, double D_0) {
    vector<pair<float, float>> path;
    float current_x = start_x, current_y = start_y;
    double v_x = 0, v_y = 0, W_x = 0, W_y = 0;

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0, 1);

    auto hypot = [](double x, double y) { return sqrt(x * x + y * y); };

    while (hypot(dest_x - start_x, dest_y - start_y) >= 1.0) {
        double dist = hypot(dest_x - start_x, dest_y - start_y);
        double W_mag = min(W_0, dist);

        if (dist >= D_0) {
            W_x = W_x / sqrt(3) + (2 * dis(gen) - 1) * W_mag / sqrt(5);
            W_y = W_y / sqrt(3) + (2 * dis(gen) - 1) * W_mag / sqrt(5);
        }
        else {
            W_x /= sqrt(3);
            W_y /= sqrt(3);

            if (M_0 < 3) {
                M_0 = dis(gen) * 3 + 3;
            }
            else {
                M_0 /= sqrt(5);
            }
        }

        v_x += W_x + G_0 * (dest_x - start_x) / dist;
        v_y += W_y + G_0 * (dest_y - start_y) / dist;

        double v_mag = hypot(v_x, v_y);
        if (v_mag > M_0) {
            double v_clip = M_0 / 2 + dis(gen) * M_0 / 2;
            v_x = (v_x / v_mag) * v_clip;
            v_y = (v_y / v_mag) * v_clip;
        }

        start_x += static_cast<float>(v_x);
        start_y += static_cast<float>(v_y);

        if (current_x != start_x || current_y != start_y) {
            path.emplace_back(start_x, start_y);
            current_x = start_x;
            current_y = start_y;
        }
    }

    return path;
}


pair<float, float> screen_to_angle(int x, int y, int screen_width, int screen_height, float fov) {
    float half_width = screen_width / 2.0f;
    float half_height = screen_height / 2.0f;
    float angle_x = atan((x - half_width) / half_width) * (fov / 2.0f);
    float angle_y = atan((y - half_height) / half_height) * (fov / 2.0f);
    return { angle_x, angle_y };
}

pair<float, float> compensate_coordinates(int x, int y, float distance, int screen_width, int screen_height, float fov) {
    auto [angle_x, angle_y] = screen_to_angle(x, y, screen_width, screen_height, fov);

    float compensated_x = angle_x * distance;
    float compensated_y = angle_y * distance;

    float compensated_screen_x = (compensated_x / (fov / 2.0f)) * (screen_width / 2.0f) + (screen_width / 2.0f);
    float compensated_screen_y = (compensated_y / (fov / 2.0f)) * (screen_height / 2.0f) + (screen_height / 2.0f);

    return { compensated_screen_x, compensated_screen_y };
}

pair<float, float> get_box_center(const cv::Rect_<float>& box) {
    float center_x = box.x + box.width / 2.0f;
    float center_y = box.y + box.height / 2.0f;
    return { center_x, center_y };
}

vector<Enemy> groupEnemies(const vector<YoloResults>& boxes) {
    // 1. Split into two lists
    vector<YoloResults> bodies;
    vector<YoloResults> heads;
    for (auto& box : boxes) {
        if (box.class_id == 0) {
            bodies.push_back(box);
        }
        else if (box.class_id == 1) {
            heads.push_back(box);
        }
    }
    
    // 2. Form the results
    vector<Enemy> result;

    // 3. Mark all the heads that have already been matched with the body
    vector<bool> headUsed(heads.size(), false);

    // 4. For each body we try to find a head
    for (auto& body : bodies) {
        // Looking for the nearest head (or head whose center is inside the body?).
        float bxCenter = body.box.x + body.box.width * 0.5f;
        float byCenter = body.box.y + body.box.height * 0.5f;

        int bestHeadIndex = -1;
        for (int i = 0; i < (int)heads.size(); i++) {
            if (headUsed[i]) continue;
            // Check the condition of belonging
            auto& hd = heads[i];
            float hxCenter = hd.box.x + hd.box.width * 0.5f;
            float hyCenter = hd.box.y + hd.box.height * 0.5f;

            // simple variant: check that (hxCenter, hyCenter) lies inside body.box
            if (body.box.contains(cv::Point2f(hxCenter, hyCenter))) {
                // we believe this head belongs to this body
                bestHeadIndex = i;
                break; // if we only want to take one head
            }
        }

        if (bestHeadIndex != -1) {
            headUsed[bestHeadIndex] = true;
            // Create an Enemy that has both a body and a head
            Enemy enemy;
            enemy.body = body;
            enemy.hasBody = true;
            enemy.head = heads[bestHeadIndex];
            enemy.hasHead = true;
            result.push_back(enemy);
        }
        else {
            // If we can't find a head, we'll add Enemy with one body anyway.
            Enemy enemy;
            enemy.body = body;
            enemy.hasBody = true;
            // hasHead = false
            result.push_back(enemy);
        }
    }

    // 5. If there are any heads left that have not been added to any body - add them separately
    for (int i = 0; i < (int)heads.size(); i++) {
        if (!headUsed[i]) {
            Enemy enemy;
            enemy.head = heads[i];
            enemy.hasHead = true;
            // hasBody = false
            result.push_back(enemy);
        }
    }

    return result;
}


// Returns the coordinate for homing or nullopt if no enemies are present
optional<pair<float, float>> find_closest_target(
    const vector<YoloResults>& boxes,
    const Monitor& monitor,
    int shootMode // 0 = auto, 1 = body, 2 = head
) {
    // 1. Group the boxes into “opponents”
    auto enemies = groupEnemies(boxes); // function from section 2

    // 2. Prepare a list of “candidate points” for each adversary
    vector<pair<float, float>> candidatePoints;

    for (auto& enemy : enemies) {
        // Counting real centers (if there is a box)
        bool hasBodyCenter = false;
        float bodyCenterX = 0.0f, bodyCenterY = 0.0f;
        if (enemy.hasBody) {
            bodyCenterX = enemy.body.box.x + enemy.body.box.width * 0.5f;
            bodyCenterY = enemy.body.box.y + enemy.body.box.height * 0.5f;
            hasBodyCenter = true;
        }

        bool hasHeadCenter = false;
        float headCenterX = 0.0f, headCenterY = 0.0f;
        if (enemy.hasHead) {
            headCenterX = enemy.head.box.x + enemy.head.box.width * 0.5f;
            headCenterY = enemy.head.box.y + enemy.head.box.height * 0.5f;
            hasHeadCenter = true;
        }

        // -- If smartGen=true, we can “customize” the head based on the body
        if (config.smartGen) {
            if (!hasHeadCenter && hasBodyCenter) {
                // 10% of the upper limit
                headCenterX = enemy.body.box.x + enemy.body.box.width * 0.5f;
                headCenterY = enemy.body.box.y + enemy.body.box.height * 0.10f;
                hasHeadCenter = true; // “generated” head
            }
        }

        // -- Next, select which points to add to candidatePoints
        if (shootMode == 2 /* head */) {
            if (hasHeadCenter) {
                candidatePoints.push_back({ headCenterX, headCenterY });
            }
        }
        else if (shootMode == 1 /* body */) {
            if (hasBodyCenter) {
                candidatePoints.push_back({ bodyCenterX, bodyCenterY });
            }
            else {
                if (!hasBodyCenter && hasHeadCenter && config.smartGen) {
                    // This is where you can “think” the body
                }
            }
        }
        else /* shootMode == 0 (auto) */ {
            // In “auto” we add both body and head (if any),
            // and if smartGen=true and something was not there, it may have already been “finalized” above.
            if (hasBodyCenter) {
                candidatePoints.push_back({ bodyCenterX, bodyCenterY });
            }
            if (hasHeadCenter) {
                candidatePoints.push_back({ headCenterX, headCenterY });
            }
        }
    }

    // 4. Èùåì áëèæàéøóþ ê öåíòðó ýêðàíà òî÷êó
    if (candidatePoints.empty()) {
        return nullopt;
    }

    // Öåíòð ýêðàíà (ãëîáàëüíûå êîîðäèíàòû)
    float screenCenterX = monitor.left + monitor.width * 0.5f;
    float screenCenterY = monitor.top + monitor.height * 0.5f;

    float minDistance = 1e30f;
    optional<pair<float, float>> closestTarget = nullopt;

    for (auto& pt : candidatePoints) {
        // Translate local (pt.x, pt.y) to global:
        // Let's assume that pt.x, pt.y already go in “global” coordinates.
        // But if you have them in ROI coordinates, you need to add monitor.left/top, etc.
        float center_x_screen = pt.first + (float)monitor.left;
        float center_y_screen = pt.second + (float)monitor.top;

        float offset_x = center_x_screen - screenCenterX;
        float offset_y = center_y_screen - screenCenterY;
        float distance = sqrt(offset_x * offset_x + offset_y * offset_y);

        if (distance < minDistance) {
            minDistance = distance;
            closestTarget = { center_x_screen, center_y_screen };
        }
    }

    return closestTarget;
}


void MoveCursor(int deltaX, int deltaY) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = deltaX;           // X offset
    input.mi.dy = deltaY;           // Y offset
    input.mi.dwFlags = MOUSEEVENTF_MOVE; // relative offset
    SendInput(1, &input, sizeof(INPUT));
}

pair<int, int> lerp(int startX, int startY, int endX, int endY, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    int x = static_cast<int>(startX + t * (endX - startX));
    int y = static_cast<int>(startY + t * (endY - startY));

    return { x, y };
}