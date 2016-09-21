#ifndef INCLUDED_SUCCINCT_GLSL
#define INCLUDED_SUCCINCT_GLSL

// --------------------------------------
// (pseudo)-succinct indexed dictionaries
// --------------------------------------

// s is a 1d GL_RG8UI, GL_RG16UI or GL_RG32UI image. 
//
// The red channel contains prefix sums to the start of each block. blue channel contains raw bits.
//
// bits is 8, 16 or 32 depending on if we're using a 8, 16 or 32 bit image
//
// returns the prefix sum of the number of 1 bits up to but not including the i'th bit in the G bit vector.
//
// This naive encoding has 2x overhead, but that is still better than 8x-32x overhead caused 
// by storing indices!

uint rank1(usampler1D s, uint i, uint bits, int lod) {
  uvec2 p = texelFetch(s, int(i >> bits), lod).rg;
  return p.r + bitCount(p.g&((1 << (i&(bits - 1))) - 1));
}

uint rank1(layout(rg32ui) uimage1D s, uint i) {
  const uint bits = 32;
  uvec2 p = imageLoad(s, int(i >> bits)).rg;
  return p.r + bitCount(p.g&((1 << (i&(bits - 1))) - 1));
}

uint rank1(layout (rg16ui) uimage1D s, uint i) {
  const uint bits = 16;
  uvec2 p = imageLoad(s,int(i>>bits)).rg;
  return p.r + bitCount(p.g&((1<<(i&(bits-1)))-1));
}

uint rank1(layout(rg8ui) uimage1D s, uint i) {
  const uint bits = 8;
  uvec2 p = imageLoad(s, int(i >> bits)).rg;
  return p.r + bitCount(p.g&((1 << (i&(bits - 1))) - 1));
}

uint rank0(usampler1D s, uint i, uint bits, int lod) {
  return i - rank1(s, i, bits, lod);
}

uint rank0(layout(rg32ui) uimage1D s, uint i) {
  return i - rank1(s, i);
}

uint rank0(layout(rg16ui) uimage1D s, uint i) {
  return i - rank1(s, i);
}

uint rank0(layout(rg8ui) uimage1D s, uint i) {
  return i - rank1(s, i);
}


#endif