#ifndef INCLUDED_SUCCINCT_GLSL
#define INCLUDED_SUCCINCT_GLSL

// --------------------------------------
// (pseudo)-succinct indexed dictionaries
// --------------------------------------

// s is a 1d GL_RG8UI, GL_RG16UI or GL_RG32UI texture. 
//
// The red channel contains prefix sums to the start of each block. blue channel contains raw bits.
//
// bits is 8, 16 or 32 depending on if we're using a 8, 16 or 32 bit texture
//
// returns the prefix sum of the number of 1 bits up to but not including the i'th bit in the G bit vector.
//
// This naive encoding has 2x overhead, but that is still better than 8x-32x overhead caused 
// by storing indices!

uint rank1(usampler1D s, uint i, uint bits) {
  uvec2 p = texelFetch(s,int(i>>bits),0).rg;
  return p.r + bitCount(p.g&((1<<(i&(bits-1)))-1));
}

uint rank0(usampler1D s, uint i, uint bits) {
  return i - rank1(s, i, bits);
}

#endif