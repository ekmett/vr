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
    float apparent_angular_radius = sun_angular_radius * turbidity;
    float c = cos(apparent_angular_radius);
    float s = sin(apparent_angular_radius);
    float LdR = dot(L,R);
    L = LdR < c ? normalize(c * L + s * normalize(R - LdR * L)) : R;
  }
  vec4 srgb_albedo = texture(diffuse_texture, uv);
  vec3 albedo = pow(srgb_albedo.xyz, vec3(2.2));
  vec3 diffuse_albedo = mix(albedo.xyz, vec3(0), rendermodel_metallic) * rendermodel_albedo;
  vec3 specular_albedo = mix(vec3(0.03f), albedo.xyz, rendermodel_metallic) * rendermodel_albedo;
  vec3 color = calc_lighting(sun_irradiance / turbidity, diffuse_albedo, specular_albedo, rendermodel_roughness, N,L,V);
  // vec3 ambient = texture(sky_cubemap, reflect(-V,N)).xyz;
  vec3 ambient = eval_sh9_irradiance(N, sky_sh9) / 3.14159f; // skylight
  ambient *= rendermodel_ambient; // proxy for sky occlusion
  color += ambient * diffuse_albedo * mix(rendermodel_albedo, 1, turbidity / 10);
  outputColor = vec4(clamp(color, 0.0f, 65000), srgb_albedo.a);
}