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
      if (vr.poll() || window.poll()) break; // local book-keeping
      compositor->WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, 0);
      auto ttp = vr.time_to_photons();
      log("app")->info("time to photons: {}", duration<float, std::milli>(ttp));
      // TODO: create a task_group for the current frame here


      glFinish();
      // present eyes here...
      compositor->PostPresentHandoff();
      // task_group.wait(); 
      // check for epoch change
    }
  }
}