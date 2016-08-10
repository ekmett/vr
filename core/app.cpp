#include <openvr.h>
#include "app.h"
#include "util.h"

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
  glClearColor(0.5, 0.5, 0.5, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void app::run() {
  tracker.hmd->CaptureInputFocus();
  bool bQuit = false;

  while (!(window.poll() || tracker.poll())) {
    render();
    // do stuff
  }  
  tracker.hmd->ReleaseInputFocus();
}
