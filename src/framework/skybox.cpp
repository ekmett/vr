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

extern "C" {
#include "ArHosekSkyModel.h"
}

#define HACK_SEASCAPE

static inline float angle_between(const glm::vec3 & x, const glm::vec3 & y) {
  return std::acosf(std::max(glm::dot(x,y), 0.00001f));
}


static float irradiance_integral(float theta) {
  float sin_theta = std::sin(theta);
  return float(M_PI) * sin_theta * sin_theta;
}

namespace framework {
  static const float cos_physical_sun_size = std::cos(physical_sun_size);

  sky::sky(const vec3 & sun_direction, float sun_size, const vec3 & ground_albedo, float turbidity)
    : rgb{}
    , cubemap(0)
    , program("sky", R"(
      #version 450 core
      #extension GL_ARB_shader_viewport_layer_array : require
      layout (location=0) uniform mat4 projection[2];
      layout (location=2) uniform mat4 model_view[2];
      const vec2 positions[4] = vec2[](vec2(-1.0,1.0),vec2(-1.0,-1.0),vec2(1.0,1.0),vec2(1.0,-1.0));
      out vec3 coord; 
      out vec3 origin;
      void main() {
        vec4 position = vec4(positions[gl_VertexID],0.0,1.0);
        mat4 inv_projection = inverse(projection[gl_InstanceID]);
        mat3 inv_model_view = transpose(mat3(model_view[gl_InstanceID]));\
        vec3 actual_origin = model_view[gl_InstanceID][3].xyz;
        actual_origin.y -= 3;
        origin = actual_origin;
        
        coord = inv_model_view * (inv_projection * position).xyz;
        gl_ViewportIndex = gl_InstanceID;
        gl_Position = position;
      }
    )", R"(
      #version 450 core
      #extension GL_ARB_bindless_texture : require
      #extension GL_ARB_shading_language_include : require // only needed if seascape is on

      layout (std140, binding=1) uniform SKY {
        vec3 sun_dir;
        vec3 sun_color;      
        samplerCube sky;
        float cos_sun_angular_radius;
      };

      in vec3 origin;
      in vec3 coord;
      out vec4 outputColor;

      vec3 getSkyColor() {
        vec3 sky_color = texture(sky, coord).xyz;
        vec3 dir = normalize(coord);
        if (cos_sun_angular_radius > 0.0f) {
          float cos_sun_angle = dot(dir, sun_dir);
          if (cos_sun_angle >= cos_sun_angular_radius)
            sky_color = sun_color;
        }
        return sky_color;
      }
    )"

#if !defined(HACK_SEASCAPE)
    R"(
      void main() {  
        vec3 sky_color = getSkyColor();
        sky_color = sky_color / (sky_color + vec3(1)); // quick tonemap for testing
        outputColor = vec4(sky_color,1.0f);
      }
    )"
#else
    R"(
      #include "seascape.h"
      void main() {
        float time = iGlobalTime * 0.3f;
        // trace the sea
        vec3 p;
        vec3 dir = normalize(coord);
        heightMapTracing(origin,dir,p);
        vec3 dist = p - origin;
        vec3 n = getNormal(p, dot(dist,dist) * EPSILON_NRM);
        vec3 light = normalize(vec3(0.0,1.0,0.8)); // TODO: use our actual sky, we have a cube map and spherical harmonics

        // color
        vec3 sea_color = getSeaColor(p,n,sun_dir,dir,dist);
        vec3 color = mix(getSkyColor(), sea_color, pow(smoothstep(-0.00,-0.01,dir.y),0.3));
        color = color / (color + vec3(1)); // tonemap for testing
        //color = clamp(color, 0.0f, 65000.0f);       
        outputColor = vec4(color,1.0f);
      } 
    )"
#endif
    ) {
    glCreateBuffers(1, &ubo);
    gl::label(GL_BUFFER, ubo, "sky ubo");
    glCreateVertexArrays(1, &vao);
    gl::label(GL_BUFFER, vao, "sky vao");
    update(sun_direction, sun_size, ground_albedo, turbidity);
  }


  // sun size is in radians, not degrees
  void sky::update(const vec3 & sun_direction_, float sun_size_, const vec3 & ground_albedo_, float turbidity_) {
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
    for (int s = 0 ; s < 6; ++s)
      for (int y = 0; y < N; ++y)
        for (int x = 0; x < N; ++x) {
          vec3 dir = xys_to_direction(x, y, s, N, N);
          vec3 radiance = sample(dir);
          int i = s*N*N + y*N + x;
          cubemap_data[i] = tvec4<half> { 
            half(radiance.r), 
            half(radiance.g), 
            half(radiance.b), 
            1.0_half
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
    cubemap_handle = glGetTextureHandleARB(cubemap);
    glMakeTextureHandleResidentARB(cubemap_handle);

    struct {
      __declspec(align(16)) vec3 sun_dir;
      __declspec(align(16)) vec3 sun_color;      
      __declspec(align(16)) GLuint64 handle;
      __declspec(align(8)) float cos_sun_angular_radius;      
    } ubo_contents{ sun_direction, sun_radiance, cubemap_handle, cos(sun_size) };

    glNamedBufferData(ubo, sizeof(ubo_contents), &ubo_contents, GL_STATIC_DRAW);
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
    glDeleteBuffers(1, &ubo);
  }

  void sky::render(const mat4 perspectives[2], const mat4 model_view[2]) const {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(program.programId);

    // Removable cruft
    glBindVertexArray(vao); // TODO: use a common vao
    glUniformMatrix4fv(0, 2, GL_FALSE, &perspectives[0][0][0]); // TODO: use a common uniform 
    glUniformMatrix4fv(2, 2, GL_FALSE, &model_view[0][0][0]);   
#ifdef HACK_SEASCAPE
    glUniform1f(4, SDL_GetTicks() / 1000.0f);
#endif
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, ubo); // place these in the big commom uniform

    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 2);
  }
}