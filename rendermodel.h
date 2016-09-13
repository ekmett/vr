#pragma once

#include "std.h"
#include "gl.h"
#include "signal.h"
#include "openvr_system.h"
#include "filesystem.h"
#include "shader.h"
#include "noncopyable.h"
#include "gui.h"
#include "fmt.h"
#include "timer.h"
#include "vao.h"

namespace framework {

  using std::string;
  using std::map;

  struct rendermodel_texture : noncopyable {
    rendermodel_texture(const string & name, vr::RenderModel_TextureMap_t & diffuse);
    ~rendermodel_texture();
    GLuint id;
    GLuint64 handle;
  };

  struct rendermodel : noncopyable {
    rendermodel(const string & name, vr::RenderModel_t & vrModel, bool missing_components = false);
    ~rendermodel();

    inline bool ready() {
      return diffuse.operator bool();
    }

    // it is made up of constituent parts, and we've loaded them.
    inline bool composite() {
      return missing_components == 0 
          && components.begin() != components.end();
    }

    vertex_array<vr::RenderModel_Vertex_t> vao;
    GLsizei vertexCount;

    string name;
    
    vr::TextureID_t vr_texture_id;           // openvr asynchronous texture id
    shared_ptr<rendermodel_texture> diffuse; // updated after the fact
    bool missing_components;                 // do we have all the component parts?
    map<string, shared_ptr<rendermodel>> components; // component parts, if any -- kinda hackish as real components don't have components right now.
  };

  
  
  // this isn't part of the tracker because it requires an opengl context
  // TODO: shove these textures into an array, all vertices and indices into a common buffer, etc.
  struct rendermodel_manager : noncopyable {
    rendermodel_manager(openvr::system & tracker);
    virtual ~rendermodel_manager();

    bool known_ready;

    void invalidate();
    bool poll(); // returns true if all models that should be are loaded.
                 // returns false if you should poll again as the model is still loading
                 // returns true if the model is present or if polling for it again won't help.
    bool poll_model(string name, shared_ptr<rendermodel> * it = nullptr, bool is_component = false);

    // returns false if you should poll again as the model is still loading
    // returns true if the model is present or if polling for it again won't help.
    // the name is supplied for debugging purposes to opengl
    bool poll_texture(string name, vr::TextureID_t, shared_ptr<rendermodel_texture> * it = nullptr);

    int model_count() { return vrrm->GetRenderModelCount(); }
    string model_name(int i);
    inline int component_count(std::string model) { return vrrm->GetComponentCount(model.c_str()); }
    string component_name(std::string model, int i);
    string component_model(std::string model, std::string cname);

    void draw(int device_mask) {
      gl::debug_group debug("rendermodels");

      static elapsed_timer timer("rendermodels");
      timer_block timed(timer);

      if (vr::VRSystem()->IsInputFocusCapturedByAnotherProcess()) return;

      glEnable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glEnable(GL_BLEND);
      // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

      glUseProgram(shader.programId);
      for (int i = vr::k_unTrackedDeviceIndex_Hmd + 1;i < vr::k_unMaxTrackedDeviceCount;++i) {
        if (device_mask & (1 << i)) {
          auto model = tracked_rendermodels[i];
          if (!model || !model->diffuse) continue;
          log("rendermodel")->info("model {}", i);
          if (model->components.empty() || model->missing_components) {
            model->vao.bind();  // sort by model?
            mat4 id(1.f);
            glProgramUniformMatrix4fv(shader, 0, 1, false, &id[0][0]);
            glProgramUniform1i(shader, 1, i);
            glProgramUniformHandleui64ARB(shader, 2, model->diffuse->handle);
            glDrawElementsInstanced(GL_TRIANGLES, model->vertexCount, GL_UNSIGNED_SHORT, nullptr, 2);
            glBindVertexArray(0);
          } else {
            glProgramUniform1i(shader, 1, i);
            vr::VRControllerState_t controller_state;
            vr::VRSystem()->GetControllerState(i, &controller_state);

            vr::RenderModel_ControllerMode_State_t controller_mode_state;
            controller_mode_state.bScrollWheelVisible = false;

            for (auto pair : model->components) {
              const string & name = pair.first;
              auto & component = pair.second;             
              vr::RenderModel_ComponentState_t component_state;
              vrrm->GetComponentState(model->name.c_str(), name.c_str(), &controller_state, &controller_mode_state, &component_state);
              if (!(component_state.uProperties & vr::VRComponentProperty_IsVisible)) continue;
              mat4 tracking_to_component = openvr::hmd_mat3x4(component_state.mTrackingToComponentRenderModel);
              component->vao.bind();
              glProgramUniformMatrix4fv(shader, 0, 1, false, &tracking_to_component[0][0]);
              glProgramUniformHandleui64ARB(shader, 2, component->diffuse->handle);
              glDrawElementsInstanced(GL_TRIANGLES, component->vertexCount, GL_UNSIGNED_SHORT, nullptr, 2);
              glBindVertexArray(0);
            }
          }
        }
      }

      glEnable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);

      glUseProgram(0);
    }
    openvr::system & tracker;
    vr::IVRRenderModels * vrrm;
    map<string, shared_ptr<rendermodel>> models;
    map<vr::TextureID_t, shared_ptr<rendermodel_texture>> textures;
    shared_ptr<rendermodel> tracked_rendermodels[vr::k_unMaxTrackedDeviceCount];
    gl::shader shader;
    scoped_connection 
      invalidate_models_on_model_skin_settings_have_changed, 
      poll_on_tracked_device_activated;
    
  };
}