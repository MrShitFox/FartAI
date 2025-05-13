#include "MainMenu.h"
#include "../imguiKit/Global.h"
#include "../config.h"

void MainMenu::Render()
{
    Manager::SetNextSize(350.0f, 250.0f);
    if (ImGui::BeginTabBar("MainMenuTabs"))
    {
        if (ImGui::BeginTabItem("Neural"))
        {
            // Save the previous state before changing
            const bool prevAimState = config.aim;

            ImGui::Checkbox("Enable Aim", &config.aim);

            // Automatic reset of aimAssist when the target is deactivated
            if (prevAimState && !config.aim && config.aimAssist)
            {
                config.aimAssist = false;
            }

            // Blocking and synchronizing aimAssist
            ImGui::BeginDisabled(!config.aim);
            ImGui::Checkbox("Aim Assist", &config.aimAssist);
            ImGui::EndDisabled();

            ImGui::SliderInt("Neural FOV", &config.neuralFov, 1, 320);
            ImGui::SliderFloat("User Smooth", &config.userSmooth, 0.001f, 1.0f, "%.3f");

            // Shoot Mode combo box
            const char* shootModeItems[] = { "Auto", "Body", "Head" };
            int currentShootMode = config.shootMode;
            if (ImGui::Combo("Shoot Mode", &currentShootMode, shootModeItems, IM_ARRAYSIZE(shootModeItems)))
            {
                config.shootMode = currentShootMode == 0 ? 0 :
                    currentShootMode == 1 ? 1 : 2;
            }

            ImGui::Checkbox("Smart Prediction", &config.smartGen);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Info"))
        {
            ImGui::Text("Info section is under construction.");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
