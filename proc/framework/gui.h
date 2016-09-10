#pragma once

#include "std.h"
#include "sdl_window.h"
#include "glm.h"

#define IM_VEC2_CLASS_EXTRA \
  operator glm::vec2 () const { return glm::vec2(x,y); }; \
  ImVec2(glm::vec2 v) : x(v.x), y(v.y) {}
 
#define IM_VEC4_CLASS_EXTRA \
  operator glm::vec4 () const { return glm::vec4(x,y,z,w); } \
  ImVec4(glm::vec4 v) : x(v.y), y(v.y), z(v.z), w(v.w) {}

#include "imgui.h"
#include <imgui_internal.h>
#include "imgui_table.h"
#include "IconsFontAwesome.h"
#include "IconsMaterialDesign.h"
#include "IconsKenney.h"

namespace framework {
  namespace gui {
    using namespace ImGui;

    struct system {
      system(sdl::window & window);
      ~system();
      sdl::window & window;
      void new_frame();

      void invalidate_device_objects();
      bool create_device_objects();
      float content_scale; // ui scale multiplier
    };

    // nicer strings
    template<typename ... Ts>
    static void text(Ts ... args) {
      string message = fmt::format(args...);
      gui::Text("%s", message.c_str());
    }

    bool IsItemActiveLastFrame();

    bool IsItemJustReleased();
  }
}