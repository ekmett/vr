#include "stdafx.h"
#include "skybox.h"
#include "glm.h"
#include <cmath>
#include <algorithm>
#include "spectrum.h"
#include "sampling.h"
#include "half.h"
#include "texturing.h"
#include "gl.h"
#include "sdl.h"
#include "std.h"
#include "gui.h"
#include "timer.h"
#include "uniforms.h"
#include <omp.h>
#include <glm/detail/type_half.hpp>

extern "C" {
#include "ArHosekSkyModel.h"
}


// #define HACK_SEASCAPE

static inline float angle_between(const glm::vec3 & x, const glm::vec3 & y) {
  return std::acosf(std::max(glm::dot(x,y), 0.00001f));
}


static float irradiance_integral(float theta) {
  float sin_theta = std::sin(theta);
  return float(M_PI) * sin_theta * sin_theta;
}

namespace framework {
  static const float cos_physical_sun_angular_radius = std::cos(physical_sun_angular_radius);

  sky::sky() // const vec3 & sun_direction, float sun_angular_radius, const vec3 & ground_albedo, float turbidity, app_uniforms & uniforms)
    : cubemap(0)
    , program("skybox")
    , direction_editor("sun_dir", "sun dir", vec3(1,0,0),true) {
    gl::debug_group debug("sky::sky");
    direction_editor.hemisphere = true;
    glCreateVertexArrays(1, &vao);
    glUniformBlockBinding(program.programId, 0, 0);
    glActiveTexture(GL_TEXTURE1);
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemap);
    glTextureParameteri(cubemap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(cubemap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(cubemap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(cubemap, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if (GLEW_ARB_seamless_cubemap_per_texture)
      glTextureParameteri(cubemap, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
    else
      log("sky")->warn("GL_ARB_seamless_cubemap_per_texture unsupported");
    glTextureStorage2D(cubemap, clamp(GLsizei(log2(float(N))),1,10), GL_RGBA16F, N, N);

    cubemap_handle = glGetTextureHandleARB(cubemap);
    glMakeTextureHandleResidentARB(cubemap_handle);

    glCreateTextures(GL_TEXTURE_2D, 6, cubemap_views);
    for (int i = 0;i < 6; ++i) {
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureStorage2D(cubemap_views[i], 3, GL_RGBA8, N, N);
    }

    initialized = false;
  }

  // sun size is in radians, not degrees
  void sky::update(app_uniforms & uniforms) {
    static int last_skybox_update_time = 0;
    static int last_solar_radiance_update_time = 0;
    static int last_update_time = 0;
    static int time_accum = 0;

    // show gui
    bool just_released = false;
    if (!initialized_direction_editor) {
      direction_editor.val = uniforms.sun_dir;
      initialized_direction_editor = true;
    }
    if (show_skybox_window) {
      gui::Begin("Skybox", &show_skybox_window);
      if (direction_editor.direction(uniforms.predicted_world_to_head)) {
        just_released = true;
      }
      gui::DragFloat("sun radius", &uniforms.sun_angular_radius, 0.05_degrees, 0.1_degrees, 15.0_degrees);
      just_released = just_released || gui::IsItemJustReleased();
      gui::SliderFloat("turbidity", &uniforms.turbidity, 1, 10, "%.2f");
      just_released = just_released || gui::IsItemJustReleased();
      gui::ColorEdit3("ground albedo", reinterpret_cast<float*>(&uniforms.ground_albedo));
      just_released = just_released || gui::IsItemJustReleased();
      gui::text("Last overall update time: {}ms", last_update_time);
      gui::text("Last solar radiance update time: {}ms", last_solar_radiance_update_time);
      gui::text("Last skybox update time: {}ms", last_skybox_update_time);
      gui::End();
    }
    uniforms.sun_dir = normalize(direction_editor.val);
    uniforms.cos_sun_angular_radius = sin(uniforms.sun_angular_radius);
    uniforms.cos_sun_angular_radius = cos(uniforms.sun_angular_radius);
    uniforms.sun_color = sun_irradiance / irradiance_integral(sun_angular_radius);

    uniforms.ground_albedo = saturate(uniforms.ground_albedo);
    uniforms.sun_angular_radius = std::max(uniforms.sun_angular_radius, 0.1_degrees);
    uniforms.turbidity = clamp(uniforms.turbidity, 1.f, 10.f);
    uniforms.sky_cubemap = cubemap_handle;

    static const float epsilon = 1e-6f;
    
    // check if we need to update
    if (initialized) { // always run if not initialized

      // skip running if the parameters are close to the current
      if (length(uniforms.sun_dir - sun_dir) < epsilon &&        
          length(uniforms.ground_albedo - ground_albedo) < epsilon &&
          uniforms.turbidity == turbidity) return;      

      // if the user is dragging the mouse rate limit so we don't spend more than half our time updating the sky
      if (!just_released) {
        time_accum += 11;
        if (time_accum < last_update_time) return;
      }
    }


    time_accum = 0;    
    int start = SDL_GetTicks(); 

    // modify cache parameters, we're doing this
    direction_editor.val = sun_dir = uniforms.sun_dir;
    sun_angular_radius = uniforms.sun_angular_radius;
    turbidity = uniforms.turbidity;
    ground_albedo = uniforms.ground_albedo;

    float theta_sun = angle_between(sun_dir, vec3(0, 1, 0));
    elevation = float(M_PI_2) - theta_sun;

    vector<tvec4<half>> cubemap_data(6 * N * N);
    vector<tvec4<uint8_t>> tonemapped_cubemap_data(6 * N*N);

    {
      // compute skybox and spherical harmonics ~2s
      {
        int sky_start = SDL_GetTicks();
        float weights = 0.0f;

        ArHosekSkyModelState * rgb[3];
        // create an rgb sky model for sampling
        for (int i = 0;i < 3;++i)
          rgb[i] = arhosek_rgb_skymodelstate_alloc_init(turbidity, ground_albedo[i], elevation);

        sh9_t<vec3> sh_array[N]{};

        #pragma omp parallel for reduction(+:weights)
        for (int y = 0; y < N; ++y) {
          for (int s = 0; s < 6; ++s) {
            for (int x = 0; x < N; ++x) {
              vec3 dir = xys_to_direction(x, y, s, N, N);
              float gamma = angle_between(dir, sun_dir);
              float theta = angle_between(dir, vec3(0, 1, 0));
              vec3 radiance {
                arhosek_tristim_skymodel_radiance(rgb[0], theta, gamma, 0),
                arhosek_tristim_skymodel_radiance(rgb[1], theta, gamma, 1),
                arhosek_tristim_skymodel_radiance(rgb[2], theta, gamma, 2),
              };
              // multiply by luminous efficiency and scale down for fp16 samples
              radiance *= 683.0f * fp16_scale;

              int i = s*N*N + y*N + x;

              cubemap_data[i] = tvec4<half>{
                half(radiance.r),
                half(radiance.g),
                half(radiance.b),
                1.0_half
              };

              bool flip[] = { false, false, true, true, false, false }; // openvr uses a different cubemap face order, apparently, with different windings

              if (flip[s]) i = (s + 1) * N*N - 1 - y*N - x;

              tonemapped_cubemap_data[i] = tvec4<uint8_t>{
                uint8_t(clamp<float>(128.0f * radiance.r / (radiance.r + 1),0.f, 255.f)),
                uint8_t(clamp<float>(128.0f * radiance.g / (radiance.g + 1),0.f, 255.f)),
                uint8_t(clamp<float>(128.0f * radiance.b / (radiance.b + 1),0.f, 255.f)),
                255
              };

              float u = (x + 0.5f) / N;
              float v = (y + 0.5f) / N;

              // map onto -1 to 1
              u = u * 2.0f - 1.0f;
              v = v * 2.0f - 1.0f;

              // compute the solid angle of the texel as weight
              const float temp = 1.0f + u*u + v*v;
              const float weight = 4.0f / (sqrt(temp) * temp);

              sh_array[y] += project_onto_sh9(dir, radiance) * weight;
              weights += weight;
            }
          }
        }

        sh9_t<vec3> sh{};
        for (int i = 0;i < N;++i) sh += sh_array[i];
        sh *= 4.0f * float(M_PI) / weights;
        for (int i = 0;i < 9;++i)
          uniforms.sky_sh9[i] = vec4(sh[i].r, sh[i].g, sh[i].b, 0);

        last_skybox_update_time = SDL_GetTicks() - sky_start;
      }
      // compute solar radiance ~15ms
      {
        int solar_start = SDL_GetTicks();
        sampled_spectrum ground_albedo_spectrum = sampled_spectrum::from_rgb(ground_albedo, spectrum_type::reflectance);

        // initialize sky_states
        ArHosekSkyModelState * sky_states[spectral_samples];
        //#pragma omp parallel for
        for (auto i = 0; i < spectral_samples; ++i)
          sky_states[i] = arhosekskymodelstate_alloc_init(theta_sun, turbidity, ground_albedo_spectrum[i]);


        // compute solar radiance
        vec3 sun_dir_x = perpendicular(sun_dir);
        mat3 sun_orientation = mat3(sun_dir_x, cross(sun_dir, sun_dir_x), sun_dir);
        const size_t num_samples = 4;
        // #pragma omp parallel for collapse(2) reduction(+:sun_irradiance)
        for (size_t x = 0;x < num_samples; ++x)
          for (size_t y = 0;y < num_samples; ++y) {
            vec3 sample_dir = sun_orientation * sample_cone(
              vec2( (x + 0.5f) / num_samples, (y + 0.5f) / num_samples ),
              cos_physical_sun_angular_radius
            );
            float sample_theta_sun = angle_between(sample_dir, vec3(0, 1, 0));
            float sample_gamma = angle_between(sample_dir, sun_dir);

            sampled_spectrum solar_radiance;

            for (size_t i = 0; i < spectral_samples; ++i)
              solar_radiance[i] = float(arhosekskymodel_solar_radiance(
                sky_states[i],
                sample_theta_sun,
                sample_gamma,
                lerp(float(sampled_lambda_start), float(sampled_lambda_end), i / float(spectral_samples))
              ));

            sun_irradiance += solar_radiance.to_rgb() * saturate<float, highp>(dot(sample_dir, sun_dir));
          }

        sun_irradiance *= (1.0f / num_samples) * (1.0f / num_samples) * (1.0f / sample_cone_pdf(cos_physical_sun_angular_radius));

        // standard luminous efficiency 683 lm/W, coordinate system scaling & scaling to fit into the dynamic range of a 16 bit float
        sun_irradiance *= 683.0f * 100.0f * fp16_scale;
        uniforms.sun_irradiance = sun_irradiance;
        uniforms.sun_color = sun_irradiance / irradiance_integral(sun_angular_radius);

        // free sky states
        for (auto i = 0; i < spectral_samples; ++i) {
          arhosekskymodelstate_free(sky_states[i]);
          sky_states[i] = nullptr;
        }

        last_solar_radiance_update_time = SDL_GetTicks() - solar_start;
      }
    }

    gl::debug_group debug("sky::update");


    // load cubemap into opengl
    glTextureSubImage3D(cubemap, 0, 0, 0, 0, N, N, 6, GL_RGBA, GL_HALF_FLOAT, cubemap_data.data());
    glGenerateTextureMipmap(cubemap);

    // right left top bottom back front - opengl order
    // front back left right top bottom - openvr order
    vr::Texture_t vr_skybox[6];
    const int swizzle[6] = { 2, 3, 4, 5, 1, 0 };

    for (int i = 0;i < 6; ++i) {
      glTextureSubImage2D(cubemap_views[i], 0, 0, 0, N, N, GL_RGBA, GL_UNSIGNED_BYTE, tonemapped_cubemap_data.data() + (N*N*i));
      glGenerateTextureMipmap(cubemap_views[i]);
      vr_skybox[swizzle[i]].handle = (void*)(intptr_t)cubemap_views[i];
      vr_skybox[swizzle[i]].eColorSpace = vr::ColorSpace_Linear;
      vr_skybox[swizzle[i]].eType = vr::API_OpenGL;
    }
    vr::VRCompositor()->SetSkyboxOverride(vr_skybox, 6);

    last_update_time = SDL_GetTicks() - start;
    initialized = true;
  }

  sky::~sky() {
    gl::debug_group debug("sky::~sky()");
    glMakeTextureHandleNonResidentARB(cubemap_handle);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &cubemap);
    glDeleteTextures(6, cubemap_views);
  }

  void sky::render() const {
    gl::debug_group debug("sky::render");
    static elapsed_timer timer("sky");
    timer_block timed(timer);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_CLAMP);
    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 2);
    glBindVertexArray(0);
    glUseProgram(0);
  }
}