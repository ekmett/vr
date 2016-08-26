#pragma once

struct SDL_Window;
typedef union SDL_Event SDL_Event;

extern bool ImGui_ImplSdlGL3_Init(SDL_Window* window);
extern void ImGui_ImplSdlGL3_Shutdown();
extern void ImGui_ImplSdlGL3_NewFrame(SDL_Window* window);
extern bool ImGui_ImplSdlGL3_ProcessEvent(SDL_Event* event);

// Use if you want to reset your rendering device without losing ImGui state.
extern void ImGui_ImplSdlGL3_InvalidateDeviceObjects();
extern bool ImGui_ImplSdlGL3_CreateDeviceObjects();
