#version 450 core
#extension GL_ARB_bindless_texture : require
#extension GL_ARB_shading_language_include : require

#include "uniforms.h"
#include "brdf.glsl"
#include "spherical_harmonics.glsl"

layout (bindless_sampler, location = 2) uniform sampler2D diffuse_texture;

in vec2 uv; // texture coord
in vec3 N_ws;
in vec3 V_ws;

out vec4 outputColor;

void main() {
  vec3 L = sun_dir;
  vec3 N = normalize(N_ws);
  vec3 V = normalize(V_ws);
  if (use_sun_area_light_approximation != 0) {
    vec3 R = reflect(V, N);
    float LdR = dot(L,R);
    vec3 S = R - LdR * L;
    L = LdR < cos_sun_angular_radius ? normalize(cos_sun_angular_radius * L + normalize(S) * sin_sun_angular_radius) : R;
  }
  vec4 srgb_albedo = texture(diffuse_texture, uv);
  vec3 albedo = pow(srgb_albedo.xyz, vec3(2.2));
  vec3 diffuse_albedo = mix(albedo.xyz, vec3(0), rendermodel_metallic) * rendermodel_albedo;
  vec3 specular_albedo = mix(vec3(0.03f), albedo.xyz, rendermodel_metallic) * rendermodel_albedo;
  vec3 color = calc_lighting(sun_irradiance, diffuse_albedo, specular_albedo, rendermodel_roughness, N,L,V);
  vec3 ambient = texture(sky_cubemap, reflect(-V,N)).xyz;
  // vec3 ambient = eval_sh9_irradiance(N, sky_sh9) / 3.14159f; // skylight
  ambient *= rendermodel_ambient; // proxy for sky occlusion
  color += ambient * diffuse_albedo * rendermodel_albedo;
  outputColor = vec4(clamp(color, 0.0f, 65000), srgb_albedo.a);
}