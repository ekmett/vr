#pragma once
#include "gl.h"
#include "gui.h"
#include "glm.h"
#include "std.h"
#include "spdlog.h"
#include "noncopyable.h"

// #define TIMING

namespace framework {

  typedef duration<GLuint64, std::nano> timestamp;

  struct timer : noncopyable {
    static int query_frame;
    static int current_frame;
    static const int N = 3; // frames worth of lag between query_frame and current_frame to ensure timers clear

    static void start_frame() {
#ifdef TIMING
      current_frame = (current_frame + 1) % N;
      if (++query_frame >= 0) query_frame %= N;
      log("timer")->info("current frame: {}, query frame: {}", current_frame, query_frame);
#endif
    }

    GLuint timestamps[N];

    timer(const char * name) {
#ifdef TIMING
      glGenQueries(N, timestamps);
      for (int i = 0;i < N;++i)
        gl::label(GL_QUERY, timestamps[i], "timer {} query {}", name, i);
#endif
    }
    timer(string name) : timer(name.c_str()) {}

    ~timer() {
#ifdef TIMING
      glDeleteQueries(N, timestamps);
#endif
    }

    void tick() const {
#ifdef TIMING
      glQueryCounter(timestamps[current_frame], GL_TIMESTAMP); // start counter i
#endif
    }

    timestamp tock() const {
#ifdef TIMING
      GLuint64 result = 0;
      glQueryCounter(timestamps[current_frame], GL_TIMESTAMP); // start counter i
      if (query_frame >= 0) {
        auto ts = timestamps[query_frame];
        GLint done;
        glGetQueryObjectiv(ts, GL_QUERY_RESULT_AVAILABLE, &done);
        if (done) {
          glGetQueryObjectui64v(ts, GL_QUERY_RESULT, &result);
        } else {
          log("timer")->warn("timing query not ready", query_frame);
        }
      }
      return std::chrono::nanoseconds(result);
#else
      return std::chrono::nanoseconds(0);
#endif
    }
  };

  struct elapsed_timer : noncopyable {
    elapsed_timer(string name) 
      : start(name + " start")
      , end(name + " end")
      , name(name) {}
    ~elapsed_timer() {}    
    timer start, end;
    string name;
  };

  struct timer_block : noncopyable {
    static int indentation;
    static bool squelch;

    timer_block(const elapsed_timer & t) 
#ifdef TIMING
      : t(t), was_squelched(squelch), pop(false) {
      t.start.tick();
      float ms = (t.end.tock() - t.start.tock()).count() / 1000000.0f;
      if (!squelch) {
        pop = gui::TreeNode(t.name.c_str(),"%s: %.02fms",t.name.c_str(), ms);
        squelch = !pop;
      } 
#endif
    {}
    ~timer_block() {
#ifdef TIMING
      if (pop) gui::TreePop();
      squelch = was_squelched;
      t.end.tick();
#endif
    }
#ifdef TIMING
    const elapsed_timer & t;
    bool pop;
    bool was_squelched;
#endif
  };
}