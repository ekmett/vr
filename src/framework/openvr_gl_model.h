#pragma once


#if 0 //wip

namespace framework {

  using std::string;
  using std::map;

  struct rendermodel : noncopyable {
    // a simple diffuse texture fetched from openvr
    struct texture : noncopyable {
      texture(const string & name, vr::RenderModel_TextureMap_t & diffuse);
      ~texture();
      GLuint id;
    };

    // this isn't part of the tracker because it requires an opengl context
    struct manager : noncopyable {
      manager(openvr_tracker & tracker);
      virtual ~manager();

      bool known_ready;

      void invalidate();
      bool poll(); // returns true if all models that should be are loaded.

                   // returns false if you should poll again as the model is still loading
                   // returns true if the model is present or if polling for it again won't help.
      bool poll_model(string name, shared_ptr<rendermodel> * it = nullptr, bool is_component = false);

      // returns false if you should poll again as the model is still loading
      // returns true if the model is present or if polling for it again won't help.
      // the name is supplied for debugging purposes to opengl
      bool poll_texture(string name, vr::TextureID_t, shared_ptr<texture> * it = nullptr);

      openvr_tracker & tracker;
      map<string, shared_ptr<rendermodel>> models;
      map<vr::TextureID_t, shared_ptr<texture>> textures;
      shared_ptr<rendermodel> tracked_rendermodels[vr::k_unMaxTrackedDeviceCount];
      shader shader;
      scoped_connection invalidate_models_on_model_skin_settings_have_changed, poll_on_tracked_device_activated;
    };

    rendermodel(const string & name, vr::RenderModel_t & vrModel, bool missing_components = false);
    ~rendermodel();

    inline bool ready() {
      return diffuse != nullptr;
    }

    // it is made up of constituent parts, and we've loaded them.
    inline bool composite() {
      return missing_components == 0
        && components.begin() != components.end();
    }

    void draw();

    GLuint vertBuffer;
    GLuint indexBuffer;
    GLuint vertArray;
    GLsizei vertexCount;

    vr::TextureID_t vr_texture_id;       // openvr asynchronous texture id
    shared_ptr<texture> diffuse;         // updated after the fact
    bool missing_components;              // do we have all the component parts?
    map<string, shared_ptr<rendermodel>> components; // component parts, if any -- kinda hackish as real components don't have components right now.
  };
}

#endif