#include "stdafx.h"
#include <random>
#include <functional>
#include "app.h"
#include "worker.h"

namespace proc {
  void app::run() {   
    pool workers(4, rng,
      [&](worker& me) { log("app")->info("worker {}: task 1", me.i); },
      [&](worker& me) { log("app")->info("worker {}: task 2", me.i); },
      [&](worker& me) { log("app")->info("worker {}: task 3", me.i); }
    );
    for (;;) {
      // TODO: create a task_group for the current frame here

      if (vr.poll() || window.poll()) break;

      // send everything to the hmd
      // post present handoff -- this is an expensive fence

      // do any local remaining work for this frame

      // task_group.wait();

      // WaitGetPoses
    }
  }
}