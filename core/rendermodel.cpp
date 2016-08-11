#include <memory>
#include "rendermodel.h"
#include "util.h"
#include "shader.h"

using namespace vr;
using namespace std;

namespace core {
  rendermodel::rendermodel(const string & name, RenderModel_t & model, bool missing_components)
    : vertexCount(model.unTriangleCount * 3)
    , diffuse(nullptr)
    , vr_texture_id(model.diffuseTextureId)
    , missing_components(missing_components) {
    glGenVertexArrays(1, &vertArray);
    glBindVertexArray(vertArray);
    objectLabelf(GL_VERTEX_ARRAY, vertArray, "%s vertex array", name.c_str());

    glGenBuffers(1, &vertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    objectLabelf(GL_BUFFER, vertBuffer, "%s vertex buffer", name.c_str());
    glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * model.unVertexCount, model.rVertexData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    objectLabelf(GL_BUFFER, indexBuffer, "%s index buffer", name.c_str());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * model.unTriangleCount * 3, model.rIndexData, GL_STATIC_DRAW);

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
    , known_ready(false)
    , invalidate_models(tracker.on_model_skin_settings_have_changed.connect([&] { invalidate(); }))
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
  
  bool rendermodel::manager::poll_texture(string name, vr::TextureID_t key, shared_ptr<texture> * result) {
    auto vrrm = VRRenderModels();   
    auto iter(textures.lower_bound(key));
    if (iter != textures.end() && key >= iter->first) {
      if (result) *result = iter->second;
      return true;
    }
    vr::RenderModel_TextureMap_t * t;
    auto err = vrrm->LoadTexture_Async(key, &t);
    switch (err) {
      case vr::VRRenderModelError_None: {
        auto m = make_shared<rendermodel::texture>(name, *t);
        vrrm->FreeTexture(t);
        textures.insert(iter, make_pair(key, m));
        if (result) *result = m;
        return true;
      }
      case vr::VRRenderModelError_Loading:
        return false;
      default:
        tracker.log->warn("Unable to load texture {}: {}", key, vrrm->GetRenderModelErrorNameFromEnum(err));
        return true;
    }
    return false;
  }

  bool rendermodel::manager::poll_model(string name, shared_ptr<rendermodel> * result, bool is_component) {
    auto vrrm = VRRenderModels();
    auto iter(models.lower_bound(name));

    // do we already have a model?
    if (iter != models.end() && name >= iter->first) {
      if (result) *result = iter->second;
      return true;
    }   

    vr::RenderModel_t *model;
    auto err = vrrm->LoadRenderModel_Async(name.c_str(), &model);
    switch (err) {
    case vr::VRRenderModelError_None: {     
      auto m = make_shared<rendermodel>(name, *model, !is_component && tracker.component_count(name) != 0);
      vrrm->FreeRenderModel(model);
      models.insert(iter, make_pair(name, m));
      if (result) *result = m;
      return true;
    }
    case vr::VRRenderModelError_Loading:
      return false;
    default:
      tracker.log->warn("Unable to load render model {}: {}", name, vrrm->GetRenderModelErrorNameFromEnum(err)); // long term failure.
      return true;
    }
  }
  
  void rendermodel::manager::invalidate() {
    known_ready = false;
    models.clear();
    textures.clear();
    poll(); 
  }

  // returns false if you should poll again as there are still parts loading
  // returns true to indicate everything is loaded.
  bool rendermodel::manager::poll() {
    if (known_ready) return true; // short circuit
    tracker.log->info("polling for render models");

    bool result = true;
    auto vrrm = vr::VRRenderModels();

    /*
    for (int i = 0, n = tracker.model_count();i < n; ++i) {
      string name = tracker.model_name(i);
      result = poll_model(name) && result;
    }
    */

    // scan only active models
    for (int i = 0;i < vr::k_unMaxTrackedDeviceCount;++i) {
      if (!tracker.valid_device(i)) continue;
      string name = tracker.device_string(i, vr::Prop_RenderModelName_String);
      if (name == "") continue;
      result = poll_model(name) && result;
    }

    // for each model
    for (auto & p : models) {
      // check for textures
      if (!p.second->diffuse)
        result = poll_texture(p.first, p.second->vr_texture_id, &p.second->diffuse) && result;
      
      // are we missing parts?
      if (p.second->missing_components) {
        bool components_found = true;
        int N = tracker.component_count(p.first);
        for (int j = 0;j < N;++j) {
          // check for the component as already added to this model first.
          auto & parts = p.second->components;
          string cname = tracker.component_name(p.first, j);
          if (cname == "") continue;
          string cmodel = tracker.component_model(p.first, cname);
          if (cmodel == "") continue;
          auto iter(parts.lower_bound(cmodel));
          if (iter != parts.end() && cmodel >= iter->first) continue; // already in there
          shared_ptr<rendermodel> part;
          bool part_found = poll_model(cmodel, &part, true);
          components_found = components_found && part_found;
          if (part_found && part != nullptr)
            parts.insert(iter, make_pair(cmodel, part));
        }
        p.second->missing_components = !components_found;
        result = components_found && result;
      }
    }
    known_ready = result;
    return result;
  }
}