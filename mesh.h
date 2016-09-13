#pragma once

#include "glm.h"
#include "obj.h"
#include "std.h"
#include "error.h"

namespace framework {
  struct mesh {
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    vec3 bmin = vec3(std::numeric_limits<float>::max());
    vec3 bmax = vec3(std::numeric_limits<float>::min());
    mesh(const char * filename) {
      string err;
      bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, nullptr);
      if (!ret) die("Failed to load {}: {}",filename, err);
      auto l = log("mesh");
      l->info("loading {}", filename);
      l->info("{} vertices, {} normals, {} texture coordinates, {} materials, {} shapes",
        attrib.vertices.size() / 3, attrib.normals.size() / 3, attrib.texcoords.size() / 3, materials.size(), shapes.size()
      );
      for (auto & s : shapes) {
        l->info("shape {}: {} indices", s.name, s.mesh.indices.size());
      }
    }
    ~mesh() {}
  };
}