#pragma once

#include "glm.h"
#include "obj.h"
#include "std.h"
#include "error.h"
#include "filesystem.h"

namespace framework {
  struct mesh {
    struct vertex {
      vec3 pos;
      vec3 normal;
      vec3 uv;
    };
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    vec3 bmin = vec3(std::numeric_limits<float>::max());
    vec3 bmax = vec3(std::numeric_limits<float>::min());
    typedef vertex_array<vertex, GL_INT> vao_type;
    vao_type vao;
    int count;
    mesh(const string & name, const char * filename)
      : vao(name, true,
          make_attrib(&vertex::pos),
          make_attrib(&vertex::normal),
          make_attrib(&vertex::uv)
      ) {
      string err;
      bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, nullptr);
      if (!ret) die("Failed to load {}: {}", filename, err);
      log("mesh")->info("loading {}: {} vertices, {} normals, {} texture coordinates, {} materials, {} shapes",
        filename,
        attrib.vertices.size() / 3,
        attrib.normals.size() / 3,
        attrib.texcoords.size() / 3,
        materials.size(),
        shapes.size()
      );
      vector<vertex> vertices;
      vector<vao_type::index_type> indices;
      map<tuple<int, int, int>, int> ids;
      auto get3 = [&](const vector<float> & v, int ix) -> vec3 {
        return make_vec3(&v[ix * 3]);
      };
      for (auto & s : shapes)
        for (auto & i : s.mesh.indices) {
          auto key = make_tuple(i.vertex_index, i.normal_index, i.texcoord_index);
          auto iter = ids.lower_bound(key);
          if (iter != ids.end() && key == iter->first) {
            indices.push_back(iter->second);
          } else {
            int id = vertices.size();
            vertices.push_back(vertex{
              get3(attrib.vertices,i.vertex_index),
              get3(attrib.normals, i.normal_index),
              get3(attrib.texcoords, i.texcoord_index)
            });
            ids.insert(iter, make_pair(key, id)); // memoize vertex
            indices.push_back(id);
          }
        }
      vao.load(vertices);
      vao.load_elements(indices);
      count = indices.size();
    }    
    mesh(const string & name) : mesh(name, filesystem::path("objects/").append(name.c_str()).replace_extension(".obj").string().c_str()) {}
    void draw() noexcept {
      vao.bind();
      glDrawElementsInstanced(GL_TRIANGLES, count, vao_type::index, nullptr, 2);
      glBindVertexArray(0);
    }

    ~mesh() {}
  };
}