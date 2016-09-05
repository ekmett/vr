#pragma once

#include "framework/std.h"
#include "framework/glm.h"
#include "framework/gl.h"

namespace framework {

  template <GLenum Target = GL_TEXTURE_2D_ARRAY, GLenum ViewTarget = GL_TEXTURE_2D> struct storage_format {
    static const GLenum target = Target;
    static const GLenum view_target = ViewTarget;
    GLsizei w, h;
    storage_format(GLsizei w = 0, GLsizei h = 0) : w(w), h(h) {};
  };

  template <typename T> struct simple_storage : storage_format<GL_TEXTURE_2D_ARRAY, GL_TEXTURE_2D> {
    simple_storage(GLsizei w = 0, GLsizei h = 0) : storage_format(w, h) {}

    void setup(T & fbo, const string & name, GLenum internalformat) const {
      glTextureStorage3D(fbo.texture, 1, internalformat, w, h, T::layer_count);
      glTextureParameteri(fbo.texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(fbo.texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(fbo.texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(fbo.texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTextureParameteri(fbo.texture, GL_TEXTURE_MAX_LEVEL, 0);
    }

    void set_view_parameters(GLuint view) {
      glTextureParameteri(view, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(view, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(view, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(view, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void teardown(T & fbo) const {
    }
  };

  template <typename T> struct multisample_storage : storage_format<GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE> {
    multisample_storage(GLsizei w = 0, GLsizei h = 0, int msaa = 0, bool fixed_sample_locations = false) 
      : storage_format(w,h)
      , msaa(msaa)
      , fixed_sample_locations(fixed_sample_locations) {}

    int msaa;
    bool fixed_sample_locations;

    void setup(T & fbo, const string & name, GLenum internalformat) const {
      glTextureStorage3DMultisample(fbo.texture, msaa, internalformat, w, h, T::layer_count, fixed_sample_locations);
    }

    void set_view_parameters(GLuint view) {}

    void teardown(T & fbo) const {}
  };

  template <size_t N = 2, template<typename> typename storage = simple_storage, typename base = noncopyable>
  struct layered_fbo : base {

    using storage_type = storage<layered_fbo>;

    static const GLenum target = storage_type::target;
    static const GLenum view_target = storage_type::view_target;
    static const size_t layer_count = N;

    layered_fbo() : initialized(false) {}

    layered_fbo(storage_type & format) : format(format), initialized(false) {}
     
    layered_fbo(storage_type & format, const string & name, GLenum internalformat = GL_RGBA16F) 
      : format(format)
      , initialized(false) {
      initialize(name, internalformat);
    }
    virtual ~layered_fbo() {
      finalize();
    }

    void bind(GLenum target = GL_FRAMEBUFFER) const {
      glBindFramebuffer(target, fbo);
    }

    void initialize(const string & name, GLenum internalformat = GL_RGBA16F) {
      finalize();

      glCreateFramebuffers(1, &fbo);
      glCreateTextures(target, 1, &texture);
      glCreateFramebuffers(N, fbo_view);
      glGenTextures(N, texture_view);

      gl::label(GL_FRAMEBUFFER, fbo, "{} fbo", name);
      gl::label(GL_TEXTURE, texture, "{} texture", name);
      
      format.setup(*this, name, internalformat); // at the least this has to set up the storage for texture, it might optionally do more
      glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, texture, 0);
      texture_handle = glGetTextureHandleARB(texture);
      glMakeTextureHandleResidentARB(texture_handle);
      gl::check_framebuffer(fbo, GL_FRAMEBUFFER);

      for (int i = 0;i < N;++i) {
        gl::label(GL_FRAMEBUFFER, fbo_view[i], "{} fbo view {}", name, i);
        glTextureView(texture_view[i], view_target, texture, internalformat, 0, 1, i, 1);
        gl::label(GL_TEXTURE, texture_view[i], "{} texture view {}", name, i);
        format.set_view_parameters(texture_view[i]);
        glNamedFramebufferTexture(fbo_view[i], GL_COLOR_ATTACHMENT0, texture_view[i], 0);
        gl::check_framebuffer(fbo_view[i], GL_FRAMEBUFFER);
      }
    }

    void finalize() {
      if (initialized) {
        for (auto h : texture_view_handle) glMakeTextureHandleNonResidentARB(h);
        glMakeTextureHandleNonResidentARB(texture_handle);
        glDeleteFramebuffers(1, &fbo);
        glDeleteFramebuffers(N, fbo_view);
        glDeleteTextures(1, &texture);
        glDeleteTextures(N, texture_view);
        initialized = false;
        format.teardown(*this);
      }
    }
  public:
    storage_type format;
    bool initialized;
    GLuint fbo, fbo_view[N];
    GLuint texture, texture_view[N];
    GLuint64 texture_handle, texture_view_handle[N];
  };

  typedef layered_fbo<2> stereo_fbo;

  template <typename T>
  struct render_storage : storage_format<GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE> {
    render_storage(GLsizei w = 0, GLsizei h = 0, int msaa = 4, bool fixed_sample_locations = true, GLenum depth_stencil_internalformat = GL_DEPTH32F_STENCIL8)
    : storage_format(w,h), msaa(msaa), fixed_sample_locations(fixed_sample_locations), depth_stencil_internalformat(depth_stencil_internalformat) {
    }
    int msaa;
    bool fixed_sample_locations;
    GLenum depth_stencil_internalformat;
    void setup(T & fbo, const string & name, GLenum internalformat) const {
      const GLsizei d = T::layer_count;
      glTextureStorage3DMultisample(fbo.texture, msaa, internalformat, w, h, d, fixed_sample_locations);
      glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, &fbo.depth_stencil_texture);
      glTextureStorage3DMultisample(fbo.depth_stencil_texture, msaa, depth_stencil_internalformat, w, h, d, fixed_sample_locations);
      glNamedFramebufferTexture(fbo.fbo, GL_DEPTH_STENCIL_ATTACHMENT, fbo.depth_stencil_texture, 0);
      fbo.depth_stencil_texture_handle = glGetTextureHandleARB(fbo.depth_stencil_texture);
      glMakeTextureHandleResidentARB(fbo.depth_stencil_texture_handle);
    }

    void set_view_parameters(GLuint view) { }

    void teardown(T & fbo) const {
      glMakeTextureHandleNonResidentARB(fbo.depth_stencil_texture_handle);
      glDeleteTextures(1, &fbo.depth_stencil_texture);
    }
  };

  struct depth_stencil_base : noncopyable {
    GLuint depth_stencil_texture;
    GLuint64 depth_stencil_texture_handle;
  };

  typedef layered_fbo<2, render_storage, depth_stencil_base> stereo_render_fbo;
};
