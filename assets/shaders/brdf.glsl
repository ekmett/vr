#ifndef INCLUDED_SHADERS_BRDF_GLSL
#define INCLUDED_SHADERS_BRDF_GLSL

// Schlick's approximation
vec3 calc_fresnel(vec3 specular_albedo, vec3 fresnel_albedo, vec3 H, vec3 L) {
  vec3 fresnel = specular_albedo + (fresnel_albedo - specular_albedo) * pow(1 - clamp(dot(L, H), 0.f, 1.f), 5.0f);
  fresnel *= clamp(dot(specular_albedo, vec3(333.0)),0.f, 1.f);
  return fresnel;
}

// common case
vec3 calc_fresnel(vec3 specular_albedo, vec3 H, vec3 L) {
  return calc_fresnel(specular_albedo, vec3(1), H, L);
}

// use smoothness s = 1 - r^2 instead? more uniform distribution, nicer in maps

// r2 = squared roughness
float GGX_V1(float r2, float NdX) {
  return 1.0f / (NdX + sqrt(r2 + (1 - r2) * NdX * NdX));
}

// Based on
// [Microfacet Models for Refraction Through Rough Surfaces](https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html)
// by Walter, Marschner, Li and Torrance (2007).
float GGX_specular(float roughness, vec3 N, vec3 H, vec3 V, vec3 L) {
  float NdH = clamp(dot(N, H), 0.f, 1.f);
  float NdL = clamp(dot(N, L), 0.f, 1.f);
  float NdV = clamp(dot(N, V), 0.f, 1.f);
  float r2 = roughness * roughness;
  float t = NdH * NdH * (r2 - 1) + 1;
  float d = r2 / (3.14159f * t * t);
  return d * GGX_V1(r2, NdL) * GGX_V1(r2, NdV);
}

// N normal dir
// L light dir
// V view dir (-I)
vec3 calc_lighting(
  vec3 peak_irradiance,
  vec3 diffuse_albedo,
  vec3 specular_albedo,
  float roughness,
  vec3 N,
  vec3 L,
  vec3 V
) {
  vec3 lighting = diffuse_albedo / 3.14159f;
  const float NdL = clamp(dot(N, L), 0.f, 1.f);
  if (NdL > 0.0f) {
    const float NdV = clamp(dot(N, V), 0.f, 1.f);
    vec3 H = normalize(L + V);
    vec3 fresnel = calc_fresnel(specular_albedo, H, L);
    float specular = GGX_specular(roughness, N, H, V, L);
    lighting += specular * fresnel;
  }
  return lighting * NdL * peak_irradiance;
}



#endif