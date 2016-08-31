#include "framework/stdafx.h"
#include "framework/error.h"
#include "framework/spdlog.h"
#include "openal.h"

namespace framework {
  namespace openal {
    system::system() {
      device = alcOpenDevice(nullptr);
      if (!device)
        die("Unable to obtain an OpenAL audio device");

      log("al")->info("device initialized: {}", alcGetString(device, ALC_DEVICE_SPECIFIER));

      context = alcCreateContext(device, nullptr);

      if (!context) {
        die("Unable to create an OpenAL context");
      }

      if (!alcMakeContextCurrent(context)) {
        alcDestroyContext(context);
        alcCloseDevice(device);
        die("Unable to activate our OpenAL context");
      }

      supports.hrtf = (bool)alcIsExtensionPresent(device, "ALC_SOFT_HRTF");
      log("al")->info("ALC_SOFT_HRTF {}supported", supports.hrtf ? "" : "not ");

      supports.stereo_angles = (bool)alIsExtensionPresent("AL_EXT_STEREO_ANGLES");
      log("al")->info("AL_EXT_STEREO_ANGLES {}supported", supports.stereo_angles ? "" : "not ");

      if (supports.hrtf) {

        alcGetStringiSOFT = reinterpret_cast<LPALCGETSTRINGISOFT>(alcGetProcAddress(device, "alcGetStringiSOFT"));
        alcResetDeviceSOFT = reinterpret_cast<LPALCRESETDEVICESOFT>(alcGetProcAddress(device, "alcResetDeviceSOFT"));

        int num_hrtf;
        /* Enumerate available HRTFs, and reset the device using one. */
        alcGetIntegerv(device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &num_hrtf);
        if (!num_hrtf)
          printf("No HRTFs found\n");
        else {
          ALCint attr[5];
          ALCint i;

          for (i = 0;i < num_hrtf;i++) {
            const ALCchar *name = alcGetStringiSOFT(device, ALC_HRTF_SPECIFIER_SOFT, i);
            hrtfs.push_back(name);
            log("al")->info("hrtf {}: {}", i, name);
          }

          log("al")->info("requesting default hrtf");


          i = 0;
          attr[i++] = ALC_HRTF_SOFT;
          attr[i++] = ALC_TRUE;
          //attr[i++] = ALC_HRTF_IF_SOFT;
          //attr[i++] = index;
          attr[i] = 0;
          if (!alcResetDeviceSOFT(device, attr))
            log("al")->warn("Failed to reset device: {}\n", alcGetString(device, alcGetError(device)));

          ALCint hrtf_state;
          /* Check if HRTF is enabled, and show which is being used. */
          alcGetIntegerv(device, ALC_HRTF_SOFT, 1, &hrtf_state);

          if (!hrtf_state) log("al")->warn("HRTF not enabled!");
          else log("al")->info("HRTF enabled, using {}", alcGetString(device, ALC_HRTF_SPECIFIER_SOFT));

          // congratulations, we have a head related transfer function!
        }
      }
    }

    system::~system() {
      log("al")->info("shutdown");
      alcMakeContextCurrent(nullptr);
      alcDestroyContext(context);
      alcCloseDevice(device);
    }
  }
}