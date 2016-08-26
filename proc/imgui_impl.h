#pragma once

#include "imgui.h"
#include "framework/sdl_window.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool ImGui_ImplSdlGL3_Init(framework::sdl::window & w);
IMGUI_API void ImGui_ImplSdlGL3_Shutdown(framework::sdl::window & w);
IMGUI_API void ImGui_ImplSdlGL3_NewFrame(framework::sdl::window & w);
// IMGUI_API bool ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_ImplSdlGL3_InvalidateDeviceObjects();
IMGUI_API bool ImGui_ImplSdlGL3_CreateDeviceObjects();
