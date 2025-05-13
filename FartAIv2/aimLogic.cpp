#include "aimLogic.h"
using namespace std;
void aimLogic() {
	bool aimLocal;
	bool aimAssistLocal;
	int neuralFovLocal;
	int shootModeLocal;
	int last_target_x = -1;
	int last_target_y = -1;
	auto [screen_width, screen_height] = get_screen_size();
	int widthCenter = screen_width / 2;
	int heightCenter = screen_height / 2;
	vector<YoloResults> previous_boxes;
	size_t previous_size = 0;
	while (true) {
		vector<YoloResults> local_boxes;
		{
			scoped_lock lock(aimMutex, aimAssistMutex, neuralFovMutex, shootModeMutex, globalResultsMutex);
			aimLocal = config.aim;
			aimAssistLocal = config.aimAssist;
			neuralFovLocal = config.neuralFov;
			shootModeLocal = config.shootMode;
			local_boxes = globalResults;
		}
		//Check aim turn on
		if (aimLocal == false) {
			Sleep(10);
			continue;
		}
		//Check aim assist turn on
		if (aimAssistLocal && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
			continue;
		}
		//Don't know
		Monitor monitor((screen_height / 2) - neuralFovLocal / 2, (screen_width / 2) - neuralFovLocal / 2, neuralFovLocal, neuralFovLocal);
		//If boxes changed
		if (!local_boxes.empty() && (local_boxes.size() != previous_size || !equal(local_boxes.begin(), local_boxes.end(), previous_boxes.begin()))) {
			//Update previous boxes for next iterations
			previous_size = local_boxes.size();
			previous_boxes = local_boxes;
			//Find closest target for move
			auto closest_target = find_closest_target(local_boxes, monitor, shootModeLocal);
			//Make massive for closest coordinates
			auto [current_target_x, current_target_y] = *closest_target;
			//AlphaS poop magic
			auto [smooth_x, smooth_y] = compensate_coordinates(current_target_x, current_target_y, 1.0f, screen_width, screen_height, 90);
			//cout << smooth_x << "  " << smooth_y << endl;
			vector<TargetCords> localTarget;
			localTarget.push_back({ smooth_x, smooth_y });
			//cout << localTarget[0].x << "  " << localTarget[0].y << endl;
			{
				lock_guard<mutex> lock(globalTargetMutex);
				globalTarget = localTarget;
			}
		}
	}
}