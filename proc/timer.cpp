#include "stdafx.h"
#include "timer.h"

namespace framework {
  int timer::current_frame = 0;
  int timer::query_frame = 1 - timer::N;
  bool timer_block::squelch = false;
}