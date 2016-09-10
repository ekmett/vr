#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include "std.h"

#ifdef _WIN32
#pragma comment(lib, "OpenAL32")
#endif

namespace framework {
  namespace openal {
    struct system {
      system();
      ~system();

      ALCdevice * device;
      ALCcontext * context;
      struct supports {
        bool hrtf;
        bool stereo_angles;
      } supports;
      vector<string> hrtfs;

      const ALCchar * (*alcGetStringiSOFT)(ALCdevice *device, ALCenum paramName, ALCsizei index);
      ALCboolean(*alcResetDeviceSOFT)(ALCdevice *device, const ALCint *attrList);
    };
  }

}