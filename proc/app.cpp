#include "stdafx.h"
#include <random>
#include <functional>
#include "app.h"
#include "framework/worker.h"

namespace proc {
  void app::run() {   
    pool workers(4, rng,
      [&](worker& me) { log("app")->info("worker {}: task 1", me.i); },
      [&](worker& me) { log("app")->info("worker {}: task 2", me.i); },
      [&](worker& me) { log("app")->info("worker {}: task 3", me.i); }
    );
    auto compositor = vr::VRCompositor();

    for (;;) {
      // TODO: create a task_group for the current frame here

      if (vr.poll() || window.poll()) break;

      // send everything to the hmd
      compositor->PostPresentHandoff();
      // post present handoff -- this is an expensive fence

      // do any local remaining work for this frame

      // task_group.wait();

      // for somebody asking for the default figure out the time from now to photons.

      auto ttp = vr.time_to_photons();
      log("app")->info("time to photons: {}", ttp);

      // WaitGetPoses
    }
  }
}