#pragma once

#include "glm.h"
#include "obj.h"
#include "std.h"
#include "error.h"
#include "filesystem.h"

namespace framework {
  struct mesh {
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    vec3 bmin = vec3(std::numeric_limits<float>::max());
    vec3 bmax = vec3(std::numeric_limits<float>::min());
    typedef vertex_array<vec3, GL_INT> vao_type;
    vao_type vao;
    mesh(const string & name, const char * filename) 
    : vao(name, true, self_attrib<vec3>()) {
      string err;
      bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, nullptr);
      if (!ret) die("Failed to load {}: {}",filename, err);
      auto l = log("mesh");
      l->info("loading {}", filename);
      l->info("{} vertices, {} normals, {} texture coordinates, {} materials, {} shapes",
        attrib.vertices.size() / 3, attrib.normals.size() / 3, attrib.texcoords.size() / 3, materials.size(), shapes.size()
      );
      vao.load(reinterpret_cast<const vec3*>(attrib.vertices.data()), attrib.vertices.size() / 3);
      vector<vao_type::index_type> indices;
      for (auto & s : shapes)
        for (auto & i : s.mesh.indices)
          indices.push_back(i.vertex_index);
      vao.load_elements(indices);
    }
    mesh(const string & name) : mesh(name, filesystem::path("objects/").append(name.c_str()).replace_extension(".obj").string().c_str()) {}
    ~mesh() {}
  };
}