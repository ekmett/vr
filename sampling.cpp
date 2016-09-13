#include "stdafx.h"
#include "math.h"
#include "sampling.h"
#include "sampling_bridson.h" // for demonstration window
#include "gui.h"
#include <random>

namespace framework {
  vec3 sample_sphere(vec2 uv) noexcept {
    float phi = uv.y * tau;
    float cosTheta = 1 - 2 * uv.x;
    float sinTheta = cos2sin(cosTheta);
    return vec3(
      cos(phi) * sinTheta, 
      sin(phi) * sinTheta, 
      cosTheta
    );
  }

  vec3 sample_sphere_cos(vec2 uv, float * pdf) noexcept {
    float phi = uv.y * tau;
    float w = 2 * uv.x - 1;
    float cosTheta = sign(w) * sqrt(abs(w));
    float sinTheta = cos2sin(cosTheta);
    if (pdf) *pdf = abs(cosTheta) * float(M_2_PI);
    return vec3(
      cos(phi) * sinTheta,
      sin(phi) * sinTheta,
      cosTheta
    );
  }

  vec3 sample_ball(vec3 uvw) noexcept {
    return sample_sphere(vec2(uvw)) * cbrt(uvw.z);
  }

  // sample hemisphere by concentric disc mapping
  vec3 sample_hemisphere(vec2 uv) noexcept {
    polar p = sample_polar(uv);
    float z = 1 - p.r * p.r;
    vec2 xy = p.to_disc();
    float d = sqrt(1 + z);
    return vec3(
      xy.x * d,
      xy.y * d,
      z
    );
  }

  // sample hemisphere with cosine weighting by concentric disc mapping
  vec3 sample_hemisphere_cos(vec2 uv, float * pdf) noexcept {
    polar p = sample_polar(uv);
    vec2 xy = p.to_disc();
    if (pdf) *pdf = sin2cos(uv.x) * float(M_1_PI); // cos theta / pi
    return vec3(xy, sin2cos(p.r));
  }

  // Archimedes' hat box
  vec3 sample_hemisphere_cos_naive1(vec2 uv) noexcept {
    float r = sqrt(glm::max(0.f, uv.x));
    float theta = uv.y * tau;
    return vec3(r * cos(theta), r * sin(theta), sqrt(glm::max(0.f, 1 - uv.x)));
  }

  vec3 sample_hemisphere_cos_naive2(vec2 uv) noexcept {
  // Generate points on a disc and project
    float phi = uv.y * tau;
    float cosTheta = sqrt(std::max(0.f, 1.0f - uv.x));
    float sinTheta = cos2sin(cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  vec3 sample_hemisphere_power_cos(vec2 uv, float exponent, float * pdf) noexcept {
    float phi = uv.y * tau;
    float cosTheta = pow(uv.x, rcp(exponent + 1));
    float sinTheta = cos2sin(cosTheta);
    if (pdf) *pdf = pow(cosTheta, exponent) * (exponent + 1) * float(0.5 * M_PI);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  vec3 sample_cone(vec2 uv, float cosThetaMax) noexcept {
    float phi = uv.y * tau;
    float cosTheta = (1.0f - uv.x) + uv.x * cosThetaMax;
    float sinTheta = cos2sin(cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  }

  // Weight for a particular GGX sample
  float sample_ggx_pdf(float roughness, vec3 N, vec3 H, vec3 V) noexcept {
    float NdH = clamp(dot(N, H), 0.f, 1.f);
    float HdV = clamp(dot(H, V), 0.f, 1.f);
    float r2 = roughness * roughness;
    float d = r2 / (pi * square(NdH * NdH * (r2 - 1) + 1));
    return (d * NdH) / (4 * HdV);
  }

  // sample a GGX distribution (in world space), optionally retrieves the pdf
  vec3 sample_ggx(float roughness, vec3 N, mat3 TtoW, vec2 uv, vec3 V, float * pdf) noexcept {
    float theta = std::atan2(roughness * std::sqrt(uv.x), std::sqrt(1 - uv.x));
    float phi = uv.y * tau;
    vec3 H = TtoW * vec3( sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta) );
    if (pdf) *pdf = sample_ggx_pdf(roughness, N, H, V);
    float HdV = abs(dot(H, V));
    return normalize(2.f * HdV * H - V);
  }


  vec2 sample_annulus(vec2 uv, float r_min, float r_max) noexcept {
    float r_min2 = r_min*r_min;
    float r_max2 = r_max*r_max;
    float theta = uv.y * tau;
    float r = sqrt(mix(r_min2, r_max2, uv.x));
    return vec2(r * cos(theta), r * sin(theta));
  }

  // ["A Low Distortion Map Between Disk and Square"](https://pdfs.semanticscholar.org/4322/6a3916a85025acbb3a58c17f6dc0756b35ac.pdf)
  // by Shirley and Chiu. Cut short before the final mapping onto the radius 1 disc so we can directly access the radius.
  //
  // Why bother? Jittered samples produce lower variance with concentric mapping.
  polar sample_polar(vec2 uv) noexcept {
    float phi = 0.f;
    float r = 0.f;
    float a = 2.f * uv.x - 1.f;
    float b = 2.f * uv.y - 1.f;
    if (a > -b) {                        // region 1 or 2
      if (a > b) {                       // region 1, also |a| > |b|        
        r = a;
        phi = pi_4 * (b / a);
      } else {                           // region 2, also |b| > |a|
        r = b;
        phi = pi_4 * (2.f - (a / b));
      }
    } else {                             // region 3 or 4      
      if (a < b) {                       // region 3, also |a| >= |b|, a != 0
        r = -a;
        phi = pi_4 * (4.f + (b / a));
      } else {                           // region 4, |b| >= |a|, but a==0 and b==0 could occur.
        r = -b;
        phi = b == 0 ? 0 : pi_4 * (6.f - (a / b));
      }
    }
    return polar{ r, phi };
  }


  // ["A Low Distortion Map Between Disk and Square"](https://pdfs.semanticscholar.org/4322/6a3916a85025acbb3a58c17f6dc0756b35ac.pdf)
  // by Shirley and Chiu.
  vec2 unsample_polar(polar p) noexcept {
    float r = p.r;
    float phi = p.phi;
    float a, b;

    if (phi < -pi_4)
      phi += tau; // range [-pi/4,7pi/4]

    if (phi < pi_4) {
      a = r;
      b = phi * a / pi_4;
    } else if (phi < 3 * pi_4) {
      b = r;
      a = (pi_2 - phi) * b / pi_4;
    } else if (phi < 5 * pi_4) {
      a = -r;
      b = (phi - pi) * a / pi_4;
    } else {
      b = -r;
      a = -(phi - 3 * pi_2) * b / pi_4;
    }
    return vec2(
      0.5 * a + 0.5,
      0.5 * b + 0.5
    );
  }

  vec2 sample_disc(vec2 uv) noexcept {
    return sample_polar(uv).to_disc();
  }

  vec2 unsample_disc(vec2 xy) noexcept {
    return unsample_polar(polar::from_disc(xy));
  }

  // The modification to include the polygon morphing modification for lens diaphram simulation from 
  // ["CryEngine3 Graphics Gems" (slide 36)](http://www.crytek.com/download/Sousa_Graphics_Gems_CryENGINE3.pdf)
  // by Tiago Sousa was drawn from [MJP's DX12 Sample Framework](http://mynameismjp.wordpress.com/) and 
  // is licensed under the MIT license.
  vec2 sample_disc(vec2 uv, float n, float blend_weight) noexcept {
    polar p = sample_polar(uv);
    float polygon_modifier = cos(pi / n) / cos(p.phi - (pi / n) * floor((n * p.phi + pi) / tau));
    p.r *= mix(1.f, polygon_modifier, blend_weight);
    return p.to_disc();
  }


  // debugging tool for sampling distributions
  void sampling_debug_window(bool * open, mat4 V) {
    gui::SetNextWindowSize(ImVec2(500, 500), ImGuiSetCond_FirstUseEver);
    if (open && gui::Begin("Distributions", open)) {
      static int e = 0;
      static int N = 1000;
      static float thetaMax = 2.7_degrees;
      static int scale = 400;
      static int variant = 0;
      static float r_min = 0.5;
      static float r_max = 1;
      static float power = 2;
      gui::SliderInt("scale", &scale, 50, 800);
      gui::SliderInt("points", &N, 0, 20000);
      gui::RadioButton("square", &e, 0); gui::SameLine();
      gui::RadioButton("sphere", &e, 1); gui::SameLine();
      gui::RadioButton("hemisphere", &e, 2);
      gui::RadioButton("hemisphere cos", &e, 3); gui::SameLine();
      gui::RadioButton("cone dir", &e, 4); gui::SameLine();
      gui::RadioButton("ball", &e, 5); gui::SameLine();
      gui::RadioButton("disc", &e, 6); gui::SameLine();
      gui::RadioButton("annulus", &e, 7);
      gui::RadioButton("sphere cos", &e, 8); gui::SameLine();
      gui::RadioButton("hemisphere power cos", &e, 9);
      static bool compute_pi_4 = true;
      if (e == 0)
        gui::Checkbox("compute pi/4", &compute_pi_4);
      if (e == 3)
        gui::SliderInt("variant", &variant, 0, 2);      
      if (e == 4)
        gui::SliderFloat("angular radius", &thetaMax, 0, 90.0_degrees);
      if (e == 7) {
        gui::SliderFloat("r_min", &r_min, 0, 1);
        gui::SliderFloat("r_max", &r_max, 0, 1);
        r_min = std::min(r_min, r_max);
        r_max = std::max(r_min, r_max);
      }
      if (e == 9) {
        gui::SliderFloat("power", &power, 0, 2048);
      }
      auto panel = [&](auto f) {
        auto draw_list = ImGui::GetWindowDrawList();
        vec2 canvas_pos = ImGui::GetCursorScreenPos();
        vec2 canvas_size = vec2(float(scale));
        vec2 canvas_end = canvas_pos + canvas_size;
        if (e == 0) {
          draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, ImColor(0.5f, 0.5f, 0.5f, 0.5f));
        } else {
          draw_list->AddCircleFilled(canvas_pos + canvas_size * 0.5f, scale * 0.5f, ImColor(0.5f, 0.5f, 0.5f, 0.5f), 40);
        }
        ImGui::InvisibleButton("sobol", canvas_size);
        float tally = 0.0f;
        int points = 0;

        auto canvas = [&](auto p) {
          return canvas_pos + canvas_size * (vec2(p) * 0.5f + 0.5f);
        };

        auto line = [&](vec3 p, vec3 q, vec3 color = vec3(1, 1, 0)) {
          draw_list->AddLine(canvas(p), canvas(q), ImColor(color.x, color.y, color.z, pow(1 - (p.z + q.z)*0.5, 1 / 2.2f)));
        };
        if (e != 0 && e != 6 && e != 7) {
          line(vec3(0, 0, 0), mat3(V)*vec3(1, 0, 0), vec3(1, 0, 0));
          line(vec3(0, 0, 0), mat3(V)*vec3(0, 1, 0), vec3(0, 1, 0));
          line(vec3(0, 0, 0), mat3(V)*vec3(0, 0, 1), vec3(0, 0, 1));
        }
        auto point = [&](vec3 p, vec3 color = vec3(1,1,0)) {
          vec3 q;
          float w;
          float f_x = 1;
          switch (e) {
            case 0: 
              q = vec3(vec2(p) * 2.0f - 1.0f, 0.5f);
              w = 1;
              f_x = !compute_pi_4 || length(vec2(p)) < 1.f ? 1.f : 0.f;
              break;
            case 1: 
              q = mat3(V) * sample_sphere(vec2(p)); 
              w = sample_sphere_pdf;
              break;

            case 2: 
              q = mat3(V) * sample_hemisphere(vec2(p)); 
              w = sample_hemisphere_pdf;
              break;
            case 3: {
              /*auto f = variant == 0 ? sample_hemisphere_cos :
                variant == 1 ? sample_hemisphere_cos_naive1 :
                sample_hemisphere_cos_naive2; */
              q = mat3(V) * sample_hemisphere_cos(vec2(p), &w);
              break;
            }
            case 4: 
              q = mat3(V) * sample_cone(vec2(p), cos(thetaMax)); 
              w = sample_cone_pdf(cos(thetaMax));
              break;
            case 5: 
              q = mat3(V) * sample_ball(p); 
              w = sample_ball_pdf;
              break;
            case 6:
              q = vec3(sample_disc(vec2(p)), 0.5f);
              w = sample_disc_pdf;
              break;
            case 7:
              q = vec3(sample_annulus(vec2(p), r_min, r_max), 0.5f);
              w = sample_annulus_pdf(r_min, r_max);
              break;
            case 8:
              q = mat3(V) * sample_sphere_cos(vec2(p), &w);
              break;
            case 9:
              q = mat3(V) * sample_hemisphere_power_cos(vec2(p), power, &w);
              break;
          }
          draw_list->AddCircleFilled(canvas(q), 1, ImColor(color.x*f_x, color.y*f_x, color.z*f_x, pow(1 - q.z, 1 / 2.2f)), 6);
          tally += w * f_x;
          ++points;
        };
        f(point);
        gui::text("~integral {}", tally / points);
        switch (e) {
          case 0: gui::text("integral {}", pi_4); break;
          case 1: gui::text("integral {}", 1 / (4 * pi)); break;    
          case 2: gui::text("integral {}", 1 / (2 * pi)); break;
        }
      };

      if (gui::CollapsingHeader("Sobol")) {
        static bool skip = true;
        gui::Checkbox("skip", &skip);
        panel([&](auto point) {
          sobol<3> seq;
          if (skip) seq.skip(N);
          for (int i = 0;i < N;++i)
            point(seq.next());
        });
      }
      if (gui::CollapsingHeader("Hammersley 2D")) {
        if (e == 5) {
          gui::Text("only 2D");
        } else {
          panel([&](auto point) {
            for (int i = 0;i < N;++i)
              point(vec3(hammersley_2d(i, N), 0));
          });
        }
      }

      if (gui::CollapsingHeader("Bridson")) {
        static float r = 0.05;
        static bool stable = true;
        static std::mt19937 rng;
        gui::SliderFloat("radius", &r, 0, 2); gui::SameLine();
        gui::Checkbox("stable", &stable);
        if (stable) rng.seed(rng.default_seed);
        if (e == 5) {
          gui::Text("only 2D");
        } else {
          panel([&](auto point) {
            bridson b(r);
            for (int i = 0;i < N;++i) {
              vec2 p;
              if (!b.next(rng, p)) {
                gui::text("{} samples", i);
                break;
              }
              point(vec3(p, 0));
            }
          });
        }
      }
      if (gui::CollapsingHeader("Mersenne")) {
        static bool stable = true;
        gui::Checkbox("stable", &stable);
        static std::mt19937 rng;
        if (stable) rng.seed(rng.default_seed);
        panel([&](auto point) {
          static std::mt19937 rng;
          uniform_real_distribution<float> dist(0.0f, 1.0f);
          for (int i = 0;i < N;++i)
            point(vec3(dist(rng), dist(rng), dist(rng)));
        });
      }

      gui::End();
    }
  }
}