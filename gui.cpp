#include "stdafx.h"
#include "config.h"
#include "shader.h"
#include "sdl_window.h"
#include "gui.h"
#include <SDL_syswm.h>

using namespace framework;

// Data
static double       g_Time = 0.0f;
static bool         g_MousePressed[3] = { false, false, false };
static float        g_MouseWheel = 0.0f;
static GLuint       g_FontTexture = 0;
static int          g_ShaderHandle = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

void render_draw_lists(ImDrawData* draw_data) {
  // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
  ImGuiIO& io = ImGui::GetIO();
  int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width == 0 || fb_height == 0)
    return;
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);

  // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);

  // Setup orthographic projection matrix
  glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
  const float ortho_projection[4][4] =
  {
    { 2.0f / io.DisplaySize.x, 0.0f,                     0.0f, 0.0f },
    { 0.0f,                    2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
    { 0.0f,                    0.0f,                    -1.0f, 0.0f },
    { -1.0f,                   1.0f,                     0.0f, 1.0f },
  };
  glProgramUniformMatrix4fv(g_ShaderHandle, 0, 1, GL_FALSE, &ortho_projection[0][0]);
  glUseProgram(g_ShaderHandle);
  glBindVertexArray(g_VaoHandle);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
  glActiveTexture(GL_TEXTURE0);
  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list = draw_data->CmdLists[n];
    const ImDrawIdx* idx_buffer_offset = 0;

    glNamedBufferData(g_VboHandle, (GLsizeiptr)cmd_list->VtxBuffer.size() * sizeof(ImDrawVert), cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
    glNamedBufferData(g_ElementsHandle, (GLsizeiptr)cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx), cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

    for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++) {
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        glBindTexture(GL_TEXTURE_2D, 0); // ensure we don't 'stutter' and keep the old texture if the new one is invalid
        glBindTexture(GL_TEXTURE_2D, (GLuint)intptr_t(pcmd->TextureId));
        glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)idx_buffer_offset);
      }
      idx_buffer_offset += pcmd->ElemCount;
    }
  }

  glUseProgram(0);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
}

static const char* get_clipboard_text() {
  return SDL_GetClipboardText();
}

static void set_clipboard_text(const char* text) {
  SDL_SetClipboardText(text);
}

static void mouse_button_down(SDL_MouseButtonEvent & e) {
  switch (e.button) {
    case SDL_BUTTON_LEFT: g_MousePressed[0] = true;
    case SDL_BUTTON_RIGHT: g_MousePressed[1] = true;
    case SDL_BUTTON_MIDDLE: g_MousePressed[2] = true;
  }
}

static void mouse_wheel(SDL_MouseWheelEvent & e) {
  if (e.y > 0) g_MouseWheel = 1;
  if (e.y < 0) g_MouseWheel = -1;
}

static void text_input(SDL_TextInputEvent & e) {
  ImGui::GetIO().AddInputCharactersUTF8(e.text);
}

static void key(bool down, SDL_KeyboardEvent & e) {
  ImGuiIO & io = ImGui::GetIO();
  int key = e.keysym.sym & ~SDLK_SCANCODE_MASK;

  io.KeysDown[key] = down;

  switch (key) {
    case SDLK_KP_ENTER: io.KeysDown[SDLK_RETURN] = down; break;
    default: break;
  }

  io.KeyShift = (SDL_GetModState() & KMOD_SHIFT) != 0;
  io.KeyCtrl = (SDL_GetModState() & KMOD_CTRL) != 0;
  io.KeyAlt = (SDL_GetModState() & KMOD_ALT) != 0;
  io.KeySuper = (SDL_GetModState() & KMOD_GUI) != 0;
}

static void key_down(SDL_KeyboardEvent & e) {
  key(true, e);
}

static void key_up(SDL_KeyboardEvent & e) {
  key(false, e);
}

void create_fonts_texture() {
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

  glCreateTextures(GL_TEXTURE_2D, 1, &g_FontTexture);
  gl::label(GL_TEXTURE, g_FontTexture, "gui font atlas");
  glTextureParameteri(g_FontTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(g_FontTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureStorage2D(g_FontTexture, 1, GL_RGBA8, width, height);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTextureSubImage2D(g_FontTexture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  //g_FontTextureHandle = glGetTextureHandleARB(g_FontTexture);
  //glMakeTextureHandleResidentARB(g_FontTextureHandle);

  // Store our identifier
  io.Fonts->TexID = (void *)(intptr_t)g_FontTexture;
}

namespace framework {
  namespace gui {
    system::system(sdl::window & window) : window(window) {
      ImGuiIO& io = ImGui::GetIO();
      io.KeyMap[ImGuiKey_Tab] = SDLK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
      io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
      io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
      io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
      io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
      io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
      io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
      io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
      io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
      io.KeyMap[ImGuiKey_Delete] = SDLK_DELETE;
      io.KeyMap[ImGuiKey_Backspace] = SDLK_BACKSPACE;
      io.KeyMap[ImGuiKey_Enter] = SDLK_RETURN;
      io.KeyMap[ImGuiKey_Escape] = SDLK_ESCAPE;
      io.KeyMap[ImGuiKey_A] = SDLK_a;
      io.KeyMap[ImGuiKey_C] = SDLK_c;
      io.KeyMap[ImGuiKey_V] = SDLK_v;
      io.KeyMap[ImGuiKey_X] = SDLK_x;
      io.KeyMap[ImGuiKey_Y] = SDLK_y;
      io.KeyMap[ImGuiKey_Z] = SDLK_z;

      io.RenderDrawListsFn = render_draw_lists;   // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
      io.SetClipboardTextFn = set_clipboard_text;
      io.GetClipboardTextFn = get_clipboard_text;

#ifdef _WIN32
      SDL_SysWMinfo wmInfo;
      SDL_VERSION(&wmInfo.version);
      SDL_GetWindowWMInfo(window.sdl_window, &wmInfo);
      io.ImeWindowHandle = wmInfo.info.win.window;
#else
      (void)window;
#endif

      window.on_text_input.connect(&text_input);
      window.on_mouse_button_down.connect(&mouse_button_down);
      window.on_mouse_wheel.connect(&mouse_wheel);
      window.on_key_down.connect(&key_down);
      window.on_key_up.connect(&key_up);

      // generalities are out of the way, now let's work on look and feel
      std::string font_dir = "fonts";

      int display = window.display_index();
      log("app")->info("SDL window is on monitor {}", display);

      float ddpi;
      SDL_GetDisplayDPI(display, &ddpi, nullptr, nullptr); // 96 default, 144 is 50% scale up
      content_scale = ddpi / 96.0f;

      log("app")->info("Display DPI: {}, scale factor: {}%", ddpi, content_scale*100.0);

      ImFontConfig config;
      config.OversampleH = 3;
      config.OversampleV = 1;
      config.PixelSnapH = true;

      /*
      {
        static const ImWchar ranges[] = { 0x0020, 0x00FF, 0 };// Basic Latin + Latin Supplement
        io.Fonts->AddFontFromFileTTF((font_dir + "DejaVuSans.ttf").c_str(), 13 * content_scale, &config, ranges);
      }
      config.MergeMode = true;
      {
        static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; // TODO: tighten.
        io.Fonts->AddFontFromFileTTF((font_dir + "fontawesome-webfont.ttf").c_str(), 15 * content_scale, &config, icons_ranges);
      }
      {
        static const ImWchar icons_ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 }; // TODO: tighten
        io.Fonts->AddFontFromFileTTF((font_dir + "MaterialIcons-Regular.ttf").c_str(), 15 * content_scale, &config, icons_ranges);
      }
      {
        static const ImWchar icons_ranges[] = { ICON_MIN_KI, ICON_MAX_KI, 0 }; // TODO: tighten
        io.Fonts->AddFontFromFileTTF((font_dir + "kenney-icon-font.ttf").c_str(), 15.0f, &config, icons_ranges);
      }
      */

      ImGuiStyle& style = ImGui::GetStyle();
      style.WindowPadding.x = 4;
      style.WindowPadding.y = 6;
      style.WindowRounding = 3;
      style.GrabRounding = 10;
      style.FrameRounding = 16;
      style.ScrollbarSize = 18;
      style.GrabMinSize = 20;            
    }

    void system::new_frame() {
      if (!g_FontTexture) create_device_objects();

      ImGuiIO& io = ImGui::GetIO();

      // Setup display size (every frame to accommodate for window resizing)
      int w, h;
      int display_w, display_h;
      SDL_GetWindowSize(window.sdl_window, &w, &h);
      SDL_GL_GetDrawableSize(window.sdl_window, &display_w, &display_h);
      io.DisplaySize = ImVec2((float)w, (float)h);
      io.DisplayFramebufferScale = ImVec2(w > 0 ? ((float)display_w / w) : 0, h > 0 ? ((float)display_h / h) : 0);

      // Setup time step
      Uint32 time = SDL_GetTicks();
      double current_time = time / 1000.0;
      io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f / 60.0f);
      g_Time = current_time;

      // Setup inputs
      // (we already got mouse wheel, keyboard keys & characters from SDL_PollEvent())
      int mx, my;
      Uint32 mouseMask = SDL_GetMouseState(&mx, &my);
      if (SDL_GetWindowFlags(window.sdl_window) & SDL_WINDOW_MOUSE_FOCUS)
        io.MousePos = ImVec2((float)mx, (float)my);   // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
      else
        io.MousePos = ImVec2(-1, -1);

      io.MouseDown[0] = g_MousePressed[0] || (mouseMask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0; // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
      io.MouseDown[1] = g_MousePressed[1] || (mouseMask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
      io.MouseDown[2] = g_MousePressed[2] || (mouseMask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
      g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;

      io.MouseWheel = g_MouseWheel;
      g_MouseWheel = 0.0f;

      // Hide OS mouse cursor if ImGui is drawing it
      SDL_ShowCursor(io.MouseDrawCursor ? 0 : 1);

      // Start the frame
      ImGui::NewFrame();
    }

    system::~system() {
      window.on_key_up.disconnect(&key_up);
      window.on_key_down.disconnect(&key_down);
      window.on_mouse_wheel.disconnect(&mouse_wheel);
      window.on_mouse_button_down.disconnect(&mouse_button_down);
      window.on_text_input.disconnect(&text_input);

      invalidate_device_objects();
      ImGui::Shutdown();
    }

    void system::invalidate_device_objects() {
      if (g_VaoHandle) glDeleteVertexArrays(1, &g_VaoHandle);
      if (g_VboHandle) glDeleteBuffers(1, &g_VboHandle);
      if (g_ElementsHandle) glDeleteBuffers(1, &g_ElementsHandle);
      g_VaoHandle = g_VboHandle = g_ElementsHandle = 0;

      glDeleteProgram(g_ShaderHandle);
      g_ShaderHandle = 0;
      if (g_FontTexture) {
        glDeleteTextures(1, &g_FontTexture);
        ImGui::GetIO().Fonts->TexID = 0;
        g_FontTexture = 0;
      }
    }
    bool system::create_device_objects() {
    
      g_ShaderHandle = gl::compile("gui");

      glCreateBuffers(1, &g_VboHandle);
      gl::label(GL_BUFFER, g_VboHandle, "gui vbo");

      glCreateBuffers(1, &g_ElementsHandle);
      gl::label(GL_BUFFER, g_ElementsHandle, "gui elements");

      glCreateVertexArrays(1, &g_VaoHandle);
      gl::label(GL_VERTEX_ARRAY, g_VaoHandle, "gui vao");

      for (int i = 0;i < 3;++i) {
        glEnableVertexArrayAttrib(g_VaoHandle, i);
        glVertexArrayAttribBinding(g_VaoHandle, i, 0);
      }

      glVertexArrayAttribFormat(g_VaoHandle, 0, 2, GL_FLOAT, GL_FALSE, offsetof(ImDrawVert, pos));
      glVertexArrayAttribFormat(g_VaoHandle, 1, 2, GL_FLOAT, GL_FALSE, offsetof(ImDrawVert, uv));
      glVertexArrayAttribFormat(g_VaoHandle, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(ImDrawVert, col));

      glVertexArrayVertexBuffer(g_VaoHandle, 0, g_VboHandle, 0, sizeof(ImDrawVert));

      create_fonts_texture();

      return true;
    }

    bool IsItemActiveLastFrame() {
      ImGuiContext& g = *GImGui;
      if (g.ActiveIdPreviousFrame)
        return g.ActiveIdPreviousFrame == g.CurrentWindow->DC.LastItemId;
      return false;
    }

    bool IsItemJustReleased() {
      return IsItemActiveLastFrame() && !ImGui::IsItemActive();
    }
}
}

