#include "framework/stdafx.h"
#include "skybox.h"
#include "framework/glm.h"
#include <cmath>
#include <algorithm>
#include "framework/spectrum.h"
#include "framework/sampling.h"
#include <glm/detail/type_half.hpp>
#include "framework/half.h"
#include "framework/texturing.h"
#include "framework/gl.h"
#include "framework/gui.h"
#include "uniforms.h"

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
    : rgb{}
    , cubemap(0)
    , program("skybox") {
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
    glTextureStorage2D(cubemap, 7, GL_RGBA16F, N, N);

    cubemap_handle = glGetTextureHandleARB(cubemap);
    glMakeTextureHandleResidentARB(cubemap_handle);

    glCreateTextures(GL_TEXTURE_2D, 6, cubemap_views);
    for (int i = 0;i < 6; ++i) {
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureStorage2D(cubemap_views[i], 7, GL_RGBA8, N, N);
    }

    initialized = false;
  }

  // sun size is in radians, not degrees
  void sky::update(app_uniforms & uniforms) {
    if (show_skybox_window) {
      gui::Begin("Skybox", &show_skybox_window);
      gui::ColorEdit3("ground albedo", reinterpret_cast<float*>(&uniforms.ground_albedo));
      gui::SliderFloat("sun size", &sun_angular_radius, 0.1_degrees, 10.0_degrees);
      gui::SliderFloat("x", &uniforms.sun_dir.x, -1, 1);
      gui::SliderFloat("y", &uniforms.sun_dir.y, 0, 1);
      gui::SliderFloat("z", &uniforms.sun_dir.z, -1, 1);
      gui::SliderFloat("turbidity", &uniforms.turbidity, 1, 32);
      gui::End();
    }

    if (initialized &&
        uniforms.sun_dir == sun_dir && 
        uniforms.sun_angular_radius == sun_angular_radius && 
        uniforms.ground_albedo == ground_albedo && 
        uniforms.turbidity == turbidity)
      return;

    sun_dir = uniforms.sun_dir = normalize(uniforms.sun_dir);
    sun_angular_radius = uniforms.sun_angular_radius = std::max(uniforms.sun_angular_radius, 0.1_degrees);
    turbidity = uniforms.turbidity = clamp(uniforms.turbidity, 1.f, 32.f);
    ground_albedo = uniforms.ground_albedo = saturate(uniforms.ground_albedo);
    uniforms.cos_sun_angular_radius = sin(uniforms.sun_angular_radius);
    uniforms.cos_sun_angular_radius = cos(uniforms.sun_angular_radius);
    uniforms.sky_cubemap = cubemap_handle;

    float theta_sun = angle_between(sun_dir, vec3(0, 1, 0));
    elevation = M_PI_2 - theta_sun;

    for (int i = 0;i < 3;++i)
      rgb[i] = arhosek_rgb_skymodelstate_alloc_init(turbidity, ground_albedo[i], elevation);    

    sampled_spectrum ground_albedo_spectrum = sampled_spectrum::from_rgb(ground_albedo, spectrum_type::reflectance);

    ArHosekSkyModelState * sky_states[spectral_samples];
    for (auto i = 0; i < spectral_samples; ++i)
      sky_states[i] = arhosekskymodelstate_alloc_init(theta_sun, turbidity, ground_albedo_spectrum[i]);

    vec3 sun_dir_x = perpendicular(sun_dir);
    mat3 sun_orientation = mat3(sun_dir_x, cross(sun_dir, sun_dir_x), sun_dir);

    const size_t num_samples = 8;
    for (size_t x = 0;x < num_samples; ++x)
      for (size_t y = 0;y < num_samples; ++y) {
        vec3 sample_dir = sun_orientation * sample_direction_cone(
          (x + 0.5f) / num_samples,
          (y + 0.5f) / num_samples,
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
        
        vec3 sample_radiance = solar_radiance.to_rgb();

        sun_irradiance += sample_radiance * saturate<float, highp>(dot(sample_dir, sun_dir));
      }

    sun_irradiance *= (1.0f / num_samples) * (1.0f / num_samples) * (1.0f / sample_direction_cone_PDF(cos_physical_sun_angular_radius));

    // standard luminous efficiency 683 lm/W, coordinate system scaling & scaling to fit into the dynamic range of a 16 bit float
    sun_irradiance *= 683.0f * 100.0f * fp16_scale;

    uniforms.sun_irradiance = sun_irradiance;

    for (auto i = 0; i < spectral_samples; ++i) {
      arhosekskymodelstate_free(sky_states[i]);
      sky_states[i] = nullptr;
    }
    
    uniforms.sun_color = sun_irradiance / irradiance_integral(sun_angular_radius);

    sh = sh9_t<vec3>();
    float weights = 0.0f;

    alloca_array<tvec4<half>> cubemap_data(6 * N * N);
    alloca_array<tvec4<uint8_t>> tonemapped_cubemap_data(6 * N*N);
    for (int s = 0; s < 6; ++s) {
      for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
          vec3 dir = xys_to_direction(x, y, s, N, N);
          vec3 radiance = sample(dir);
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
            uint8_t(clamp<float>(128.0 * radiance.r / (radiance.r + 1),0.f, 255.f)),
            uint8_t(clamp<float>(128.0 * radiance.g / (radiance.g + 1),0.f, 255.f)),
            uint8_t(clamp<float>(128.0 * radiance.b / (radiance.b + 1),0.f, 255.f)),
            255
          };

          float u = (x + 0.5f) / N;
          float v = (y + 0.5f) / N;

          // map onto -1 to 1
          u = u * 2.0f - 1.0f;
          v = v * 2.0f - 1.0f;

          // account for distribution
          const float temp = 1.0f + u*u + v*v;
          const float weight = 4.0f / (sqrt(temp) * temp);

          sh += project_onto_sh9(dir, radiance) * weight;
          weights += weight;
        }
      }
    }
    sh *= 4.0f * float(M_PI) / weights;

    for (int i = 0;i < 9;++i) {
      uniforms.sky_sh9[i] = vec4(sh[i].r, sh[i].g, sh[i].b, 0);
    }

    log("sky")->info("spherical harmonics: {}", sh);

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

    initialized = true;
  }

  vec3 sky::sample(const vec3 & dir) const {
    float gamma = angle_between(dir, sun_dir);
    float theta = angle_between(dir, vec3(0, 1, 0));
    vec3 radiance{
      arhosek_tristim_skymodel_radiance(rgb[0], theta, gamma, 0),
      arhosek_tristim_skymodel_radiance(rgb[1], theta, gamma, 1),
      arhosek_tristim_skymodel_radiance(rgb[2], theta, gamma, 2),
    };
    // multiply by luminous efficiency and scale down for fp16 samples
    return radiance * 683.0f * fp16_scale;
  }

  sky::~sky() {
    for (auto m : rgb) arhosekskymodelstate_free(m);
    glMakeTextureHandleNonResidentARB(cubemap_handle);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &cubemap);
    glDeleteTextures(6, cubemap_views);
  }

  void sky::render() const {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // glDisable(GL_CULL_FACE);
    glUseProgram(program);
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);
    glBindVertexArray(0);
    glUseProgram(0);
  }
}