#pragma once

#include "std.h"
#include "glm.h"
#include "gui.h"
#include "math.h"
#include <glm/gtx/euler_angles.hpp>

namespace framework {
  enum struct direction_input_mode {
    cartesian,
    spherical
  };


  struct direction_setting {
    direction_setting(const char * name, const char * label, vec3 val, bool convert_to_view_space)
      : val(val)
      , old_val(val)
      , convert_to_view_space(convert_to_view_space)
      , label(label)
      , name(name) {
      child_name = this->name + "_child";
      button_name = this->name + "_button";
    }

    ~direction_setting() {}

    vec3 val;
    vec3 old_val;
    vec2 spherical;
    vec2 last_drag_delta = vec2(0);
    bool hemisphere = false;

    bool was_dragged = false;
    bool convert_to_view_space = false;
    bool changed = false;
   
    string label;
    string name;
    string child_name;
    string button_name;
    string help_text = "";

    direction_input_mode input_mode = direction_input_mode::cartesian;

    bool direction(const mat4 & view) {
      bool just_released = false;
      mat3 view3 = convert_to_view_space ? mat3(view) : mat3(1.0f);
      const float widget_size = 75.0f;
      vec2 text_size = ImGui::CalcTextSize(label.c_str());
      ImGui::BeginChild(child_name.c_str(), vec2(0.0f, widget_size + text_size.y * 4.5f), true);
      ImGui::BeginGroup();
      ImGui::Columns(2, nullptr, false);
      ImGui::SetColumnOffset(1, widget_size + 35.0f);
      ImGui::Text(label.c_str());
      ImGui::InvisibleButton(button_name.c_str(), vec2(widget_size, widget_size));
      just_released = just_released || gui::IsItemJustReleased();
      if (ImGui::IsItemActive()) {
        vec2 drag_delta = ImGui::GetMouseDragDelta();
        vec2 rotAmt = drag_delta;

        if (was_dragged)
          rotAmt -= last_drag_delta;
        rotAmt *= 0.01f;
        mat3 rotation = mat3(eulerAngleXY(rotAmt.y, rotAmt.x));
        val = val * view3 * rotation * transpose(view3);
        was_dragged = true;
        last_drag_delta = drag_delta;
      } else
        was_dragged = false;

      if (hemisphere) {
        val.y = clamp(val.y, 0.f, 1.f);
        val = normalize(val);
      }

      vec2 canvas_start = ImGui::GetItemRectMin();
      vec2 canvas_size = ImGui::GetItemRectSize();
      vec2 canvas_end = ImGui::GetItemRectMax();

      auto canvas = [&](auto pos) -> vec3 {
        vec3 start = vec3(canvas_start, 0);
        vec3 size = vec3(canvas_size, 1);
        return start + size * (vec3(pos) * vec3(0.5f, -0.5f, 0.4f) + vec3(0.5f,0.5f,0.6f));
      };
           
      ImDrawList* drawList = ImGui::GetWindowDrawList();

      auto line = [&](vec3 a, vec3 b) {
        float z = (a.z + b.z)/2;
        z = pow(z, 2.2);
        ImColor color(1.0f, 1.0f, 0.f, z);
        drawList->AddLine(vec2(a), vec2(b), color);
      };

      drawList->AddCircleFilled(canvas_start + widget_size * 0.5f, widget_size * 0.5f, ImColor(0.5f, 0.5f, 0.5f, 0.8f), 32);

      vec3 draw_dir = view3 * val;
      vec3 draw_dir_x = perpendicular(draw_dir);
      mat3 basis = mat3(draw_dir_x, cross(draw_dir, draw_dir_x), draw_dir);

      const float arrow_head_size = 0.1f;

      vec3 arrow_head_points[] = {
        vec3(0.0f, 0.0f, 1.0f) + vec3(1.0f, 0.0f, -1.0f) * arrow_head_size,
        vec3(0.0f, 0.0f, 1.0f) + vec3(0.0f, 1.0f, -1.0f) * arrow_head_size,
        vec3(0.0f, 0.0f, 1.0f) + vec3(-1.0f, 0.0f, -1.0f) * arrow_head_size,
        vec3(0.0f, 0.0f, 1.0f) + vec3(0.0f, -1.0f, -1.0f) * arrow_head_size,
      };   

      auto rot = [&](float theta, float r = 1.0f) { return canvas(view3 * r * vec3(cos(theta), 0, -sin(theta))); };

      if (convert_to_view_space) {
        line(rot(0, 0.9), rot(0, 1.1));
        float lo = .7f;
        float hi = .85f;
        float w = float(-M_PI / 30);
        line(rot(-w, lo), rot(-w, hi)); // draw a "north" symbol at azimuth 0, elevation 0
        line(rot(-w, hi), rot(w, lo));
        line(rot(w, lo), rot(w, hi));
        vec3 o = rot(0);
        for (int i = 1;i <= 60;++i) {
          vec3 n = rot(i * float(2 * M_PI / 60));
          line(o, n);
          o = n;
        }
      }
      line(canvas(view3 * vec3(0, 1, 0)), canvas(view3 * vec3(0, 0, 0)));

      vec3 start_point = canvas(0.0f);
      vec3 end_point = canvas(draw_dir);

      // rough z ordering
      if (end_point.z >= 0) 
        line(start_point, end_point);

      for (uint64 i = 0; i < countof(arrow_head_points); ++i) {
        vec3 head_point = canvas(basis * arrow_head_points[i]);
        line(head_point, end_point);
        uint64 nextHeadIdx = (i + 1) % countof(arrow_head_points);
        vec3 next_head_point = canvas(basis * arrow_head_points[nextHeadIdx]);
        line(head_point, next_head_point);
      }
      if (end_point.z < 0)
        line(start_point, end_point);

      ImGui::NextColumn();

      switch (input_mode) {
        case direction_input_mode::cartesian:
          ImGui::SliderFloat("x", &val.x, -1.0f, 1.0f);
          just_released = just_released || gui::IsItemJustReleased();
          ImGui::SliderFloat("y", &val.y, hemisphere ? 0.0f : 1.0f, 1.0f);
          just_released = just_released || gui::IsItemJustReleased();
          ImGui::SliderFloat("z", &val.z, -1.0f, 1.0f);
          just_released = just_released || gui::IsItemJustReleased();
          spherical = cartesian_to_spherical(val);
          break;
        case direction_input_mode::spherical:
          vec2 degrees = radians_to_degrees(spherical);
          ImGui::SliderFloat("azimuth", &degrees.x, 0.0f, 360.0f);
          just_released = just_released || gui::IsItemJustReleased();
          ImGui::SliderFloat("elevation", &degrees.y, hemisphere ? 0.0f : -90.0f, 90.0f);
          just_released = just_released || gui::IsItemJustReleased();
          spherical = degrees_to_radians(degrees);
          val = spherical_to_cartesian(spherical);
          break;
      }

      bool cartesianButton = ImGui::RadioButton("Cartesian", input_mode == direction_input_mode::cartesian);
      ImGui::SameLine();
      bool sphericalButton = ImGui::RadioButton("Spherical", input_mode == direction_input_mode::spherical);

      if (cartesianButton)
        input_mode = direction_input_mode::cartesian;
      else if (sphericalButton)
        input_mode = direction_input_mode::spherical;
      ImGui::EndGroup();
      if (ImGui::IsItemHovered() && !help_text.empty())
        ImGui::SetTooltip("%s", help_text.c_str());
      ImGui::EndChild();
      val = normalize(val);
      changed = old_val != val;
      old_val = val;
      return just_released;
    }    
  };
}

// The direction editing control is drawn from 
// [MJP's DX11 Sample Framework](http://mynameismjp.wordpress.com/)
// Licensed under the MIT license
