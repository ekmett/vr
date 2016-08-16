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

    // TODO: spin up extra threads for dealing with the network and file IO so that we don't have to ever block a worker.

    auto compositor = vr::VRCompositor();

    // we are the ui thread. only things that have to be done in the ui thread should be done here from now on.
    for (;;) {
      if (vr.poll() || window.poll()) break; // local book-keeping

      // TODO: this should be done properly so that workers see only a consistent snapshot without synchronization
      compositor->WaitGetPoses(physical_pose, vr::k_unMaxTrackedDeviceCount, predicted_pose, 0);
      // 
      auto ttp = vr.time_to_photons();
      log("app")->info("time to photons: {}", duration<float, std::milli>(ttp));
      // TODO: create a task_group for the current frame here

      // draw stuff

      // present it...
      glFinish();
      compositor->PostPresentHandoff();
      // task_group.wait(); 
      // check for epoch change
    }
  }
}