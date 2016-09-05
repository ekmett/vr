#pragma once

#include "framework/std.h"
#include "framework/sdl_window.h"
#include "imgui.h"
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
  }
}