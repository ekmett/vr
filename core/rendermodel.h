#pragma once

#include <memory>
#include <string>
#include <gl/glew.h>
#include <SDL_opengl.h>
#include <GL/glu.h>
#include <openvr.h>
#include "signal.h"
#include "tracker.h"
#include "shader.h"
#include "noncopyable.h"

namespace core {

  struct rendermodel : noncopyable {
    // a simple diffuse texture fetched from openvr
    struct texture : noncopyable {
      texture(const std::string & name, vr::RenderModel_TextureMap_t & diffuse);
      ~texture();
      GLuint id;
    };

    // this isn't part of the tracker because it requires an opengl context
    struct manager : noncopyable {
      manager(openvr_tracker & tracker);
      virtual ~manager();

      // returns true if we managed to load all the render models, false if there are some we need to wait for still
      bool scan();

      openvr_tracker & tracker;
      std::map<std::string, std::shared_ptr<rendermodel>> models;
      std::map<vr::TextureID_t, std::shared_ptr<texture>> textures;
      std::shared_ptr<rendermodel> tracked_rendermodels[vr::k_unMaxTrackedDeviceCount];
      shader shader;
    };

    rendermodel(const std::string & name, vr::RenderModel_t & vrModel);
    ~rendermodel();

    inline bool ready() {
      return diffuse != nullptr;
    }

    void draw();

    GLuint vertBuffer;
    GLuint indexBuffer;
    GLuint vertArray;
    GLsizei vertexCount;

    vr::TextureID_t vr_texture_id; // openvr asynchronous texture id
    std::shared_ptr<texture> diffuse; // updated after the fact
  };

}