#include <random>
#include "std.h"
#include "glm.h"
namespace framework {

  // convenience function
  template <typename T, typename RNG> inline T uniform_int(RNG rng, T l, T h) {
    std::uniform_int_distribution<T> d(l, h);
    return d(rng);
  }

  // for additional shape-based rejection sampling.
  inline bool unit_bounds_check(const vec2 & candidate) noexcept {
    return candidate.x >= 0 
        && candidate.x <= 1 
        && candidate.y >= 0 
        && candidate.y <= 1;
  }

  // 2d fast poisson disk sampling
  struct bridson {
    const float r;
    const size_t N = sqrt(2) / r;
    // TODO: if the grid is large and the number of samples expected is small we should switch to a map, this can take crushing amounts of memory
    vector<int> grid = vector<int>(N*N, -1);
    vector<int> active;
    vector<vec2> samples;
    bool initialized = false;
    int tries = 15;
    static std::uniform_real_distribution<float> runif;
    
    bridson(float r) noexcept : r(std::max(r, 0.001f)) {}

    template <typename RNG, typename OK = bool(const vec2 &)>
    bool next(RNG & rng, vec2 & result, OK ok = unit_bounds_check) noexcept {
      if (!initialized) {
        result = vec2(runif(rng), runif(rng));
        samples.push_back(result);
        active.push_back(0);
        grid[cell(result)] = 0;
        initialized = true;
        return true;
      }
      while (!active.empty()) { 
        size_t index = uniform_int<size_t>(rng, 0, active.size() - 1); // pick a random active sample
        vec2 center = samples[active[index]];
        for (int t = 0;t < tries;++t) {
          vec2 uv = vec2(runif(rng), runif(rng));
          vec2 candidate = center + sample_annulus(uv, r, 2 * r);
          if (!ok(candidate)) continue;
          size_t x = pos(candidate.x), y = pos(candidate.y); 
          for (int i = bound(x - 2), i_max = bound(x + 2); i <= i_max; ++i) {
            for (int j = bound(y - 2), j_max = bound(y + 2); j <= j_max; ++j) {
              int g = grid[N * i + j];
              if (g != -1 && length(samples[g] - candidate) <= r) {
                goto reject; // too close to an existing sample
              }
            }
          }
          int candidate_index = samples.size();
          samples.push_back(candidate);
          active.push_back(candidate_index);
          grid[N * x + y] = candidate_index;
          result.x = candidate.x;
          result.y = candidate.y;
          return true;
        reject: continue;
        }
        // deactivate this sample
        active[index] = active.back();
        active.pop_back();
      }     
      // no active samples
      return false;
    }
    int bound(int x) const noexcept {
      return clamp<int>(x, 0, N - 1);
    }

    int pos(float x) const noexcept {
      return bound(floor(x * N));
    }

    size_t cell(vec2 sample) const noexcept {
      return N * pos(sample.x) + pos(sample.y);
    }
  };

  template <typename RNG, typename BOUNDS_CHECK, typename F>
  void for_bridson(float radius, RNG rng, F f) {

  }
}