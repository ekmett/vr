#pragma once

#include "imgui.h"

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_API bool ImGui_ImplSdlGL3_Init(SDL_Window* window);
IMGUI_API void ImGui_ImplSdlGL3_Shutdown();
IMGUI_API void ImGui_ImplSdlGL3_NewFrame(SDL_Window* window);
IMGUI_API bool ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_API void ImGui_ImplSdlGL3_InvalidateDeviceObjects();
IMGUI_API bool ImGui_ImplSdlGL3_CreateDeviceObjects();
