#ifndef INCLUDED_SUCCINCT_GLSL
#define INCLUDED_SUCCINCT_GLSL

// --------------------------------------
// (pseudo)-succinct indexed dictionaries
// --------------------------------------

// [Space-Efficient, High-Performance Rank & Select Structures on Uncompressed Bit Sequences](https://www.cs.cmu.edu/~dga/papers/zhou-sea2013.pdf)
// by Zhou, Andersen, and Kaminsky, but modified to use texture loads, and remove L0.
struct poppy {
  // for every 2k block we store a cumulative prefix sum up to the block (L12.r)
  // and non cumulative prefix sums within the block for each 512 bits, 10 bits each, 
  // packed into a 32 bit member as rgb10_a2ui
  layout(rg32ui) readonly uimage1D L12;
  layout(rgba32ui) readonly uimage1D raw;
};

// 2k blocks, 3.15% overhead, no L0, so there is a 2^32 entry, 512mb limit
// but 64 bit ints aren't well supported in glsl anyways
uint rank1(poppy p, uint i) {
  uvec2 header = imageLoad(p.L12, int(i >> 11)).rg;
  int subblock = int(i >> 9);
  int subblock_base = subblock << 2;
  uint m = header.g & ((1 << (10 * (subblock & 3))) - 1); // mask off the parts we want to keep
  uint subblock_count = m & 1023 + (m >> 10) & 1023 + (m >> 20) & 1023; // SWAR prefix sum

  uvec4 data[4];
  for (int j = 0;j < 4;++j) // use j <= z ? save half the loads on average
    data[j] = imageLoad(p.raw, subblock_base + j);

  int z = int(i >> 7) & 3; // uvec4 within subblock
  int w = int(i >> 5) & 3; // uint within uvec4
  data[z][w] &= (1 << (i & 31)) - 1;

  ivec4 counts[4];
  int last = 0;
  for (int j = 0;j < 4;++j) { // use j <= z ?
    counts[j] = bitCount(data[j]);
    counts[j].zw += counts[j].xy;
    counts[j].yzw += counts[j].xyz; // prefix sum w/in uvec4
    counts[j] += last;              // prefix sum w/in subblock
    last = counts[j].w;
  }
  return header.r + subblock_count + counts[z][w];
}

uint rank0(poppy p, uint i) {
  return i - rank1(p, i);
}

#endif