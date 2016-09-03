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
  static const float cos_physical_sun_size = std::cos(physical_sun_size);

  sky::sky(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity, app_uniforms & uniforms)
    : rgb{}
    , cubemap(0)
    , program("skybox") {
    update(sun_direction, sun_size, ground_albedo, turbidity, uniforms);
    glUniformBlockBinding(program.programId, 0, 0);
  }

  // sun size is in radians, not degrees
  void sky::update(const vec3 & sun_direction_, float sun_size_, const vec3 & ground_albedo_, float turbidity_, app_uniforms & uniforms) {
    vec3 new_sun_direction = sun_direction_;
    new_sun_direction.y = (new_sun_direction.y);
    new_sun_direction = normalize(new_sun_direction);
    float new_sun_size = max(sun_size_, 0.1_degrees);
    float new_turbidity = clamp(turbidity_, 1.f, 32.f);
    vec3 new_ground_albedo = saturate(ground_albedo_);

    if (new_sun_direction == sun_direction && new_sun_size == sun_size && new_ground_albedo == ground_albedo && new_turbidity == turbidity)
      return;

    float theta_sun = angle_between(sun_direction, vec3(0, 1, 0));
    elevation = M_PI_2 - theta_sun;
    sun_direction = new_sun_direction;
    sun_size = new_sun_size;
    turbidity = new_turbidity;
    ground_albedo = new_ground_albedo;
    log("sky")->info("beginning update");

    for (int i = 0;i < 3;++i) {
      rgb[i] = arhosek_rgb_skymodelstate_alloc_init(turbidity, ground_albedo[i], elevation);
    }

    sampled_spectrum ground_albedo_spectrum = sampled_spectrum::from_rgb(ground_albedo);

    ArHosekSkyModelState * sky_states[spectral_samples];
    for (auto i = 0; i < spectral_samples; ++i)
      sky_states[i] = arhosekskymodelstate_alloc_init(theta_sun, turbidity, ground_albedo_spectrum[i]);

    vec3 sun_direction_x = perpendicular(sun_direction);
    mat3 sun_orientation = mat3(sun_direction_x, cross(sun_direction, sun_direction_x), sun_direction);

    const size_t num_samples = 8;
    for (size_t x = 0;x < num_samples; ++x)
      for (size_t y = 0;y < num_samples; ++y) {
        vec3 sample_dir = sun_orientation * sample_direction_cone(
          (x + 0.5f) / num_samples,
          (y + 0.5f) / num_samples,
          cos_physical_sun_size
        );
        float sample_theta_sun = angle_between(sample_dir, vec3(0, 1, 0));
        float sample_gamma = angle_between(sample_dir, sun_direction);

        sampled_spectrum solar_radiance;
        for (size_t i = 0; i < spectral_samples; ++i) {
          float wavelength = lerp(float(sampled_lambda_start), float(sampled_lambda_end), i / float(spectral_samples));
          solar_radiance[i] = float(arhosekskymodel_solar_radiance(sky_states[i], sample_theta_sun, sample_gamma, wavelength));
        }
        vec3 sample_radiance = solar_radiance.to_rgb();

        sun_irradiance += sample_radiance * saturate<float, highp>(dot(sample_dir, sun_direction));
      }

    float pdf = sample_direction_cone_PDF(cos_physical_sun_size);
    sun_irradiance *= (1.0f / num_samples) * (1.0f / num_samples) * (1.0f / pdf);

    // standard luminous efficiency 683 lm/W, coordinate system scaling & scaling to fit into the dynamic range of a 16 bit float
    sun_irradiance *= 683.0f * 100.0f * fp16_scale;
  
    for (auto i = 0; i < spectral_samples; ++i) {
      arhosekskymodelstate_free(sky_states[i]);
      sky_states[i] = nullptr;
    }
    
    // compute uniform solar radiance value
    sun_radiance = sun_irradiance / irradiance_integral(sun_size);

    log("sky")->info("computed solar radiance: {}", sun_radiance);

    sh = sh9_t<vec3>();
    float weights = 0.0f;

    static const int N = 128;
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

          bool flip[] = { false, false, true, true, false, false };

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
    sh *= (4.0f * float(M_PI)) / weights;

    log("sky")->info("spherical harmonics: {}", sh);

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
    glTextureSubImage3D(cubemap, 0, 0, 0, 0, N, N, 6, GL_RGBA, GL_HALF_FLOAT, cubemap_data.data());
    glGenerateTextureMipmap(cubemap);

    // copy the data for openvr
    glCreateTextures(GL_TEXTURE_2D, 6, cubemap_views);   
    
    // right left top bottom back front - opengl order
    // front back left right top bottom - openvr order
    vr::Texture_t vr_skybox[6];
    // int swizzle[6] = { 5, 4, 1, 0, 2, 3 };
    int swizzle[6] = { 2, 3, 4, 5, 1, 0 };
    //int swizzle[6] = { 1, 0, 3, 2, 5, 4}; //
    for (int i = 0;i < 6; ++i) {
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(cubemap_views[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureStorage2D(cubemap_views[i], 7, GL_RGBA8, N, N);
      glTextureSubImage2D(cubemap_views[i], 0, 0, 0, N, N, GL_RGBA, GL_UNSIGNED_BYTE, tonemapped_cubemap_data.data() + (N*N*i));
      glGenerateTextureMipmap(cubemap_views[i]);
      vr_skybox[swizzle[i]].handle = (void*)(intptr_t)cubemap_views[i];
      vr_skybox[swizzle[i]].eColorSpace = vr::ColorSpace_Gamma; // well, not really
      vr_skybox[swizzle[i]].eType = vr::API_OpenGL;
    }
    vr::VRCompositor()->SetSkyboxOverride(vr_skybox, 6);

    cubemap_handle = glGetTextureHandleARB(cubemap);
    glMakeTextureHandleResidentARB(cubemap_handle);

    uniforms.sun_dir = sun_direction;
    uniforms.sun_color = sun_radiance;
    uniforms.sky_cubemap = cubemap_handle;
    uniforms.cos_sun_angular_radius = cos(sun_size);
  }

  vec3 sky::sample(const vec3 & dir) const {
    float gamma = angle_between(dir, sun_direction);
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
    glDeleteTextures(1, &cubemap);
    glDeleteTextures(6, cubemap_views);
  }

  void sky::render() const {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(program.programId);
    // assume we're bound to the dummy vao
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);
  }
}