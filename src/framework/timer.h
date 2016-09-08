#pragma once
#include "framework/gl.h"
#include "framework/gui.h"
#include "framework/glm.h"
#include "framework/std.h"
#include "framework/spdlog.h"
#include "framework/noncopyable.h"

namespace framework {

  typedef duration<GLuint64, std::nano> timestamp;

  struct timer : noncopyable {
    static int query_frame;
    static int current_frame;
    static const int N = 3; // frames worth of lag between query_frame and current_frame to ensure timers clear

    static void start_frame() {
      current_frame = (current_frame + 1) % N;
      if (++query_frame >= 0) query_frame %= N;
      log("timer")->info("current frame: {}, query frame: {}", current_frame, query_frame);
    }

    GLuint timestamps[N];

    timer(const char * name) {
      glGenQueries(N, timestamps);
      for (int i = 0;i < N;++i)
        gl::label(GL_QUERY, timestamps[i], "timer {} query {}", name, i);
    }
    timer(string name) : timer(name.c_str()) {}

    ~timer() {
      glDeleteQueries(N, timestamps);
    }

    void tick() const {
      glQueryCounter(timestamps[current_frame], GL_TIMESTAMP); // start counter i
    }

    timestamp tock() const {
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

    timer_block(const elapsed_timer & t) : t(t), was_squelched(squelch), pop(false) {
      t.start.tick();
      float ms = (t.end.tock() - t.start.tock()).count() / 1000000.0f;
      if (!squelch) {
        pop = gui::TreeNode(t.name.c_str(),"%s: %.02fms",t.name.c_str(), ms);
        squelch = !pop;
      }
    }
    ~timer_block() {
      if (pop) gui::TreePop();
      squelch = was_squelched;
      t.end.tick();
    }
    const elapsed_timer & t;
    bool pop;
    bool was_squelched;
  };
}