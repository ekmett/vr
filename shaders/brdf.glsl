#ifndef INCLUDED_SHADERS_BRDF_GLSL
#define INCLUDED_SHADERS_BRDF_GLSL

#include "common.glsl"

// [Numerical verification of bidirectional reflectance distribution functions for physical plausibility](https://www.researchgate.net/publication/259885429_Numerical_Verification_of_Bidirectional_Reflectance_Distribution_Functions_for_Physical_Plausibility)
// notes some basic properties any BRDF should satisfy

// 1.) f_r(omega_i, omega_r) >= 0                                     -- positivity
// 2.) f_r(omega_i, omega_r) = f_r(omega_r, omega_i)                  -- Helmholtz reciprocity
// 3.) integral_Omega f_r(omega_i, omega_r) cos theta_i domega_i <= i -- conservation of energy

// ----------------------------------------------------
// Fresnel Term
// ----------------------------------------------------

// F = (1 + V.N)^lambda

// Schlick's approximation of the fresnel term
// f0 is specular albedo: the reflection coefficient for light incoming parallel to the normal vector
// f90 is fresnel albedo: the reflection coefficient for light grazing the surface
vec3 F_schlick(vec3 f0, vec3 f90, float LdH) {
  vec3 fresnel = mix(f0, f90, pow(1 - LdH, 5.0f));
  fresnel *= saturate(dot(f0, vec3(333.3f)));
  return fresnel;
}

vec3 F_schlick(vec3 f0, float LdH) {
  return F_schlick(f0, vec3(1), LdH);
}

float F_schlick(float f0, float f90, float LdH) {
  float fresnel = mix(f0, f90, pow(1 - LdH, 5.0f));
  fresnel *= saturate(f0 * 1000.0f);
  return fresnel;
}

float F_schlick(float f0, float LdH) {
  return F_schlick(f0, 1, LdH);
}

float F_cook_torrance(float f0, float LdH) {
  float sqrt_f0 = sqrt(f0);
  float eta = (1 + sqrt_f0) / (1 - sqrt_f0);
  float g = sqrt(eta*eta + LdH*LdH - 1);
  return 0.5 * sqr((g - LdH) / (g + LdH)) * (1 + sqr(((g + LdH)*LdH - 1) / ((g - LdH)*LdH + 1)));
}

// physically based reflection coefficients terms
const vec3 f0_water = vec3(0.02, 0.02, 0.02);
const vec3 f0_plastic = vec3(0.03, 0.03, 0.03);
const vec3 f0_plastic_high = vec3(0.05, 0.05, 0.05);
const vec3 f0_glass = vec3(0.08, 0.08, 0.08);
const vec3 f0_diamond = vec3(0.17, 0.17, 0.17);
const vec3 f0_iron = vec3(0.56, 0.57, 0.58);
const vec3 f0_copper = vec3(0.95, 0.64, 0.54);
const vec3 f0_gold = vec3(1.00, 0.71, 0.29);
const vec3 f0_aluminum = vec3(0.91, 0.92, 0.92);
const vec3 f0_silver = vec3(0.95, 0.93, 0.88);

// indices of refraction for common materials drawn from 
// http://hyperphysics.phy-astr.gsu.edu/hbase/tables/indrf.html
const float n_vacuum = 1;
const float n_air = 1.00029;
const float n_ice = 1.31;
const float n_water = 1.33; // at 20 degrees C
const float n_acetone = 1.36;
const float n_ethyl_alcohol = 1.36;
const float n_sugar_solution_30 = 1.38; // 30% sugar solution
const float n_fluorite = 1.433;
const float n_fused_quartz = 1.46;
const float n_glycerine = 1.473;
const float n_sugar_solution_80 = 1.49;
const float n_crown_glass = 1.52;
const float n_spectacle_crown = 1.523;
const float n_sodium_chloride = 1.54;
const float n_polystyrene = 1.55;
const float n_polystyrene_high = 1.59;
const float n_flint_glasses = 1.57;
const float n_sapphire = 1.77;
const float n_diamond = 2.417;

// compute the reflection coefficient for light incoming parallel to the normal
float calc_f0(float n1, float n2) {
  float t = (n1 - n2) / (n2 + n1);
  return t * t;
}

float calc_f0(float n2) {
  return calc_f0(n_air, n2);
}

// ----------------------------------------------------
// Specular D Term
// ----------------------------------------------------

// Beckmann's microfacet distribution.
//
// 1/pi incorporated
float D_beckmann(float smoothness, float NdH) {
  float cosAlpha = max(NdH, 0.0001f);
  float cosAlpha2 = cosAlpha*cosAlpha;              // cos^2(alpha)
  float roughness2 = 1 - smoothness;
  return exp((cosAlpha2 - 1.0f) / (cosAlpha2 * roughness2)) / (pi*cosAlpha2*cosAlpha2*roughness2);
}

// [Microfacet Models for Refraction Through Rough Surfaces](https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html)
// by Walter, Marschner, Li and Torrance (2007).
//
// Trowbridge-Reitz
// 
// 1/pi incorporated.
float D_ggx(float smoothness, float NdH) {
  float t = 1.f - NdH * NdH * smoothness;
  return (1.f - smoothness) / (3.14159f * t * t);
}

// 1/pi incorporated.
float D_blinn_phong(float smoothness, float NdH) {
  float alpha = 1 - smoothness;
  float alpha2 = alpha * alpha;
  return pow(NdH, 2 / alpha2 - 2) / (pi * alpha2);
}

float calc_smoothness(float roughness) {
  return 1 - roughness*roughness;
}

float calc_roughness(float smoothness) {
  return sqrt(1 - smoothness);
}

// ----------------------------------------------------
// Geometry Term
// ----------------------------------------------------

float rcp_G_ggx_v1(float smoothness, float NdX) {
  return NdX + sqrt(1 + smoothness * (NdX*NdX - 1));
}

float G_ggx(float smoothness, float NdL, float NdV) {
  return 1.0f / (rcp_G_ggx_v1(smoothness, NdL) * rcp_G_ggx_v1(smoothness, NdV));
}

float rcp_G_smith_g1(float roughness2, float NdX) {
  float NdX2 = NdX*NdX;
  return 1 + sqrt(1 + roughness2 * (1 - NdX2) / NdX2);
}

float G_smith(float smoothness, float NdL, float NdV) {
  float roughness2 = 1 - smoothness;
  return 4.0f / (rcp_G_smith_g1(roughness2, NdL) * rcp_G_smith_g1(roughness2, NdV));
}

float G_neumann(float NdL, float NdV) {
  return (NdL * NdV) / max(NdL, NdV);
}

float G_ward(float NdL, float NdV) {
  return inversesqrt(NdL * NdV);
}

float G_ashikhmin_shirley(float NdL, float NdV, float LdH) {
  return 1.0f / (LdH * max(NdL, NdV));
}

float G_ashikhmin_premoze(float NdL, float NdV) {
  return 1.0f / (NdL + NdV - NdL*NdV);
}

float G_kelemen(float LdH) {
  return 0.25 / sqr(LdH);
}

float G_implicit(float NdL, float NdV) {
  return NdL * NdV;
}

float G_beckmann(float NdV, float smoothness) {
  float c = NdV / ((1 - smoothness)  * cos2sin(NdV));
  float c2 = c*c;
  return c < 1.6 : (3.535*c + 2.181*c2) / (1 + 2.276*c + 2.577*c2) : 1;
};

// ----------------------------------------------------
// The Cook-Torrance BRDF
// ----------------------------------------------------

//         D*F*G
// R_s = ----------
//       (N.L)(N.V)
//
// It is alternately written with a 1/4 or 1/pi factor here the 1/pi factor is assumed baked into D.
//
// D is the microfacet slope distribution, e.g. Beckmann, GGX.
// F is the Fresnel term.
// G is the geometric attentuation factor
//
// unit vectors:
// -------------------------------------------
// V points back to the viewer
// L points to the light
// N is the surface normal
// H is the half angle vector between L and V.
//
// This is pretty much the stock video game physically based BRDF
vec3 calc_lighting(
  vec3 peak_irradiance,
  vec3 diffuse_albedo,
  vec3 f0,
  float smoothness,
  vec3 N,
  vec3 L,
  vec3 V
) {
  vec3 lighting = diffuse_albedo / 3.14159f;
  float NdL = saturate(dot(N, L));
  if (NdL > 0.0f) {
    vec3 H = normalize(L + V);
    float LdH = saturate(dot(L, H));
    float NdH = saturate(dot(N, H));
    float NdV = saturate(dot(N, V)); // move earlier + return 0 if this is negative?
    vec3  F = F_schlick(f0, LdH);
    float D = D_ggx(smoothness, NdH);
    float G = G_ggx(smoothness, NdL, NdV);
    lighting += F*D*G;
  }
  return lighting * NdL * peak_irradiance;
}

#endif