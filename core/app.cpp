#include <openvr.h>
#include "app.h"
#include "util.h"

namespace core {
  void app::render() {
    distortion.stereo([&](vr::EVREye eye) {
      /*
      bool input_captured = tracker.hmd->IsInputFocusCapturedByAnotherProcess();
      if (!input_captured) {
        // draw the controller axis lines
        glUseProgram(controller_shader.programId);
        glUniformMatrix4fv(controllerMatrixLocation, 1, GL_FALSE, GetCurrentViewProjectionMatrix(eye).get());
        glBindVertexArray(controllerVAO);
        glDrawArrays(GL_LINES, 0, controllerVertexCount);
        glBindVertexArray(0);
      }*/

    });

    SDL_GL_SwapWindow(window.window);
    glClearColor(0.f, 1.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
    // start more stuff in the background while we wait for this:
    vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0); // without doing this the compositor doesn't believe we know what we're doing.

  }

  void app::run() {
    
    // hook events on the tracker side to modify this here in this scope, todo: exponential back-off?
    if (!tracker.hmd->CaptureInputFocus())
      die("Unable to capture input focus");

    log->info("captured focus");

    bool bQuit = false;

    while (!(window.poll() || tracker.poll())) {
      rendermodels.poll();
      render();
    }
    tracker.hmd->ReleaseInputFocus();
    log->info("released focus");
  }
}