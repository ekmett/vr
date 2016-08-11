#include <memory>
#include "rendermodel.h"
#include "util.h"
#include "shader.h"

using namespace vr;
using namespace std;

namespace core {

  rendermodel::rendermodel(const string & name, RenderModel_t & model)
    : vertexCount(model.unTriangleCount * 3)
    , diffuse(nullptr)
    , vr_texture_id(model.diffuseTextureId) {
    glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);
    objectLabelf(GL_VERTEX_ARRAY, vertArray, "%s vertex array", name.c_str());

    glGenBuffers(1, &vertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * model.unVertexCount, model.rVertexData, GL_STATIC_DRAW);
    objectLabelf(GL_BUFFER, vertBuffer, "%s vertex buffer", name.c_str());

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * model.unTriangleCount * 3, model.rIndexData, GL_STATIC_DRAW);
    objectLabelf(GL_BUFFER, indexBuffer, "%s index buffer", name.c_str());

    glBindVertexArray(0);
  }

  rendermodel::~rendermodel() {
    glDeleteBuffers(1, &indexBuffer);
    glDeleteVertexArrays(1, &vertArray);
    glDeleteBuffers(1, &vertBuffer);
  }

  void rendermodel::draw() {
    if (!diffuse) return;
    glBindVertexArray(vertArray);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuse->id);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
  }


  rendermodel::texture::texture(const std::string & name, vr::RenderModel_TextureMap_t & diffuse) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    objectLabelf(GL_TEXTURE, id, "%s texture", name.c_str());

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, diffuse.unWidth, diffuse.unHeight,
      0, GL_RGBA, GL_UNSIGNED_BYTE, diffuse.rubTextureMapData);

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

    // let it go..
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  rendermodel::texture::~texture() {
    glDeleteTextures(1, &id);
  }

  // returns true if we managed to load all the render models, false if there are some we need to wait for still

  rendermodel::manager::manager(openvr_tracker & tracker)
    : tracker(tracker)
    , shader("model",
      R"(#version 410
		   uniform mat4 matrix;
		   layout(location = 0) in vec4 position;
		   layout(location = 1) in vec3 v3NormalIn;
		   layout(location = 2) in vec2 v2TexCoordsIn;
		   out vec2 v2TexCoord;
		   void main() {
		     v2TexCoord = v2TexCoordsIn;
		     gl_Position = matrix * vec4(position.xyz, 1);
		   })",
      R"(#version 410 core
		   uniform sampler2D diffuse;
		   in vec2 v2TexCoord;
		   out vec4 outputColor;
		   void main() {
		     outputColor = texture( diffuse, v2TexCoord);
		   })") {

  }

  rendermodel::manager::~manager() {}

  bool rendermodel::manager::scan() {
    bool ok = true;
    auto vrrm = vr::VRRenderModels();

    // scan for unknown render models
    for (int i = 0;i < vr::k_unMaxTrackedDeviceCount;++i) {
      // invalid device?
      if (tracker.hmd->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_Invalid) continue;

      string name = tracker.model_name(i);
      auto iter(models.lower_bound(name));

      // do we already have a model?
      if (iter != models.end() && name >= iter->first) {
        tracked_rendermodels[i] = iter->second;
        continue;
      }

      vr::RenderModel_t *model;
      auto err = vrrm->LoadRenderModel_Async(name.c_str(), &model);
      switch (err) {
      case vr::VRRenderModelError_None: {
        auto m = make_shared<rendermodel>(name, *model);
        models.insert(iter, make_pair(name, m));
        tracked_rendermodels[i] = m;
        break;
      }
      case vr::VRRenderModelError_Loading:
        ok = false;
        continue;
      default:
        tracker.log->warn("Unable to load render model {}: {}", name, vrrm->GetRenderModelErrorNameFromEnum(err));
        continue;
      }
    }

    // scan render models for unknown textures
    for (auto && p : models) { // in c++17 this'd be nicer.

      // it already knows its texture?
      if (p.second->diffuse) continue;

      auto key = p.second->vr_texture_id;
      // check cache
      auto iter(textures.lower_bound(key));
      if (iter == textures.end() || key < iter->first) {
        vr::RenderModel_TextureMap_t * t;
        auto err = vrrm->LoadTexture_Async(key, &t);
        switch (err) {
        case vr::VRRenderModelError_None:
          textures.insert(iter, make_pair(key, make_shared<rendermodel::texture>(p.first, *t)));
          p.second->diffuse = iter->second;
          break;
        case vr::VRRenderModelError_Loading:
          ok = false; // we still need to bake more textures
          continue;
        default:
          tracker.log->warn("Unable to load texture {} for render model {}: {}", key, p.first, vrrm->GetRenderModelErrorNameFromEnum(err));
          continue;
        }
      } else {
        p.second->diffuse = iter->second;
        // found it!
      }
    }
    return ok;
  }
}