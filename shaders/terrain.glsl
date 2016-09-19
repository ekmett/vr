#ifndef INCLUDED_SHADERS_TERRAIN_GLSL
#define INCLUDED_SHADERS_TERRAIN_GLSL

// Use NVidia's spherical edge tessellation technique to compute a level of detail for a tessellated edge based on the 
// [DirectX 11 Terrain Tessellation Whitepaper](https://developer.nvidia.com/sites/default/files/akamai/gamedev/files/sdk/11/TerrainTessellation_WhitePaper.pdf)
// by Iain Cantlay.
float edge_tessellation_factor(float scale, vec2 viewport_size, mat4 P, mat4 MV, vec3 p0, vec3 p1) {
  vec4 center_vs = MV * vec4(0.5 * (p0 + p1), 1);
  vec4 r_vs = vec4(0.5 * distance(p0, p1), 0, 0, 0);
  vec4 left = P * (center_vs - r_vs);
  vec4 right = P * (center_vs + r_vs);
  left /= left.w;
  right /= right.w;
  left.xy *= viewport_size;
  right.xy *= viewport_size;
  return clamp(distance(left.xy, right.xy) * scale, 1.0f, 64.0f);
}

#endif