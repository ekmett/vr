#include "stdafx.h"
#include "rendermodel.h"
#include "shader.h"
#include "filesystem.h"
#include "glm.h"

using namespace vr;
using namespace std;

namespace framework {
  rendermodel::rendermodel(const string & name, RenderModel_t & model, bool missing_components)
    : vertexCount(model.unTriangleCount * 3)
    , diffuse(nullptr)
    , vr_texture_id(model.diffuseTextureId)
    , name(name)
    , missing_components(missing_components)
    , vao(name, true, 
      make_attrib(&vr::RenderModel_Vertex_t::vPosition),
      make_attrib(&vr::RenderModel_Vertex_t::vNormal),
      make_attrib(&vr::RenderModel_Vertex_t::rfTextureCoord)
    ) {
    vao.load(model.rVertexData, model.unVertexCount);
    vao.load_elements(model.rIndexData, model.unTriangleCount * 3);
  }

  rendermodel::~rendermodel() {}

  rendermodel_texture::rendermodel_texture(const std::string & name, vr::RenderModel_TextureMap_t & diffuse) {
    log("rendermodel")->info("create texture {} begin: {} x {}", name, diffuse.unWidth, diffuse.unHeight);
    glCreateTextures(GL_TEXTURE_2D, 1, &id);
    glTextureStorage2D(id, 5, GL_RGBA8, diffuse.unWidth, diffuse.unHeight);
    glTextureSubImage2D(id, 0, 0, 0, diffuse.unWidth, diffuse.unHeight, GL_RGBA, GL_UNSIGNED_BYTE, diffuse.rubTextureMapData);
    gl::label(GL_TEXTURE, id, "{} texture", name);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    glTextureParameterf(id, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

    glGenerateTextureMipmap(id);

    handle = glGetTextureHandleARB(id);
    glMakeTextureHandleResidentARB(handle);
    log("rendermodel")->info("create texture end", name);
  }

  rendermodel_texture::~rendermodel_texture() {
    glMakeTextureHandleNonResidentARB(handle);
    glDeleteTextures(1, &id);
  }

  // returns true if we managed to load all the render models, false if there are some we need to wait for still

  rendermodel_manager::rendermodel_manager(openvr::system & tracker)
    : tracker(tracker)
    , known_ready(false)
    , invalidate_models_on_model_skin_settings_have_changed(tracker.on_model_skin_settings_have_changed.connect([&] { 
        log("rendermodel")->info("model skin settings have changed");
        invalidate();
      }))
    , poll_on_tracked_device_activated(tracker.on_tracked_device_activated.connect([&] { 
        log("rendermodel")->info("tracked device activated");
        known_ready = false; poll();
      }))
    , shader("rendermodel")
    , vrrm(vr::VRRenderModels()) {
    glUniformBlockBinding(shader.programId, 0, 0);
  }

  rendermodel_manager::~rendermodel_manager() {}
  
  bool rendermodel_manager::poll_texture(string name, vr::TextureID_t key, shared_ptr<rendermodel_texture> * result) {
    auto iter(textures.lower_bound(key));
    if (iter != textures.end() && key >= iter->first) {
      if (result) *result = iter->second;
      return true;
    }
    string nice_name = filesystem::path(name).filename().generic_string();

    vr::RenderModel_TextureMap_t * t;
    auto err = vrrm->LoadTexture_Async(key, &t);
    switch (err) {
      case vr::VRRenderModelError_None: {
        auto m = make_shared<rendermodel_texture>(name, *t);
        vrrm->FreeTexture(t);
        textures.insert(iter, make_pair(key, m));
        if (result) *result = m;
        log("rendermodel")->info("loaded texture {} (key {})", nice_name, key);
        return true;
      }
      case vr::VRRenderModelError_Loading:
        log("rendermodel")->info("texture {} (key {}) is still loading...", nice_name, key);
        return false;
      default:
        log("rendermodel")->warn("Unable to load texture {}: {}", key, vrrm->GetRenderModelErrorNameFromEnum(err));
        return true;
    }
    return false;
  }

  bool rendermodel_manager::poll_model(string name, shared_ptr<rendermodel> * result, bool is_component) {
    auto iter(models.lower_bound(name));

    // do we already have a model?
    if (iter != models.end() && name >= iter->first) {
      if (result) *result = iter->second;
      return true;
    }   

    string nice_name = filesystem::path(name).filename().generic_string();

    vr::RenderModel_t *model;
    auto err = vrrm->LoadRenderModel_Async(name.c_str(), &model);
    switch (err) {
    case vr::VRRenderModelError_None: {     
      auto m = make_shared<rendermodel>(name, *model, !is_component && component_count(name) != 0);
      vrrm->FreeRenderModel(model);
      models.insert(iter, make_pair(name, m));
      if (result) *result = m;
      log("rendermodel")->info("loaded {} {}", is_component ? "component" : "model", nice_name);
      return true;
    }
    case vr::VRRenderModelError_Loading:
      log("rendermodel")->info("{} {} is still loading...", is_component ? "component" : "model", nice_name);
      return false;
    default:
      log("rendermodel")->warn("Unable to load rendermodel {}: {}", nice_name, vrrm->GetRenderModelErrorNameFromEnum(err)); // long term failure.
      return true;
    }
  }
  
  void rendermodel_manager::invalidate() {
    known_ready = false;
    models.clear();
    textures.clear();
    for (int i = 0;i < countof(tracked_rendermodels);++i) tracked_rendermodels[i] = nullptr;
    poll(); 
  }

  // returns false if you should poll again as there are still parts loading
  // returns true to indicate everything is loaded.
  bool rendermodel_manager::poll() {
    // if (known_ready) return true; // short circuit
    //log("rendermodel")->info("polling for render models");

    bool result = true;

    // precache everything?
    /*
    for (int i = 0, n = model_count();i < n; ++i) {
      string name = model_name(i);
      result = poll_model(name) && result;
    }
    */

    // scan only active models
    for (auto i = vr::k_unTrackedDeviceIndex_Hmd + 1;i < vr::k_unMaxTrackedDeviceCount;++i) {
      if (!tracker.valid_device(i)) continue;
      string name = tracker.device_string(i, vr::Prop_RenderModelName_String);
      if (name == "") continue;
      result = poll_model(name, &tracked_rendermodels[i]) && result;
    }

    // for each model
    for (auto & p : models) {
      // check for textures
      if (!p.second->diffuse)
        result = poll_texture(p.first, p.second->vr_texture_id, &p.second->diffuse) && result;
      
      // are we missing parts?
      if (p.second->missing_components) {
        bool components_found = true;
        int N = component_count(p.first);
        for (int j = 0;j < N;++j) {
          // check for the component as already added to this model first.
          auto & parts = p.second->components;
          string cname = component_name(p.first, j);
          if (cname == "") continue;
          string cmodel = component_model(p.first, cname);
          if (cmodel == "") continue;
          auto iter(parts.lower_bound(cmodel));
          if (iter != parts.end() && cmodel >= iter->first) continue; // already in there
          shared_ptr<rendermodel> part;
          bool part_found = poll_model(cmodel, &part, true);
          components_found = components_found && part_found && part && part->diffuse;
          if (part_found && part != nullptr)
            parts.insert(iter, make_pair(cname, part));
        }
        p.second->missing_components = !components_found;
        result = components_found && result;
      }
    }
    known_ready = result;
    return result;
  }

  string rendermodel_manager::model_name(int i) {
    return openvr::buffered(mem_fn(&IVRRenderModels::GetRenderModelName), vrrm, i);
  }

  string rendermodel_manager::component_name(string model, int i) {
    return openvr::buffered(mem_fn(&IVRRenderModels::GetComponentName), vrrm, model.c_str(), i);
  }

  string rendermodel_manager::component_model(string model, string cname) {
    return openvr::buffered(mem_fn(&IVRRenderModels::GetComponentRenderModelName), vrrm, model.c_str(), cname.c_str());
  }
}