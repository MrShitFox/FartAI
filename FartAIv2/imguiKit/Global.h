#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <iostream>

#include "../imgui/imgui_impl_dx11.h"
#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui.h"

#include "Render.h"
#include "Style.h"
#include "../Windows/WindowBase.h"
#include "Manager.h"

#include "../Windows/LoginWindow.h"
#include "../Windows/MainMenu.h"


namespace Global
{
    inline bool ShouldExit = false;
}