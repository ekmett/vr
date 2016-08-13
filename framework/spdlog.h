#pragma once

#include "std.h"

#if defined(_UNICODE)
#define SPDLOG_WCHAR_FILENAMES
#endif

#include <spdlog/spdlog.h> // only retrieve this through logger.h, so that SPDLOG_WCHAR_FILENAMES is set consistently
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/fmt/ostr.h>

namespace framework {
  using spdlog::logger;
  using spdlog::filename_t;

#if defined(_DEBUG) && defined(_WIN32)
  using default_sink = spdlog::sinks::msvc_sink_mt;
#else
  using default_sink = spdlog::sinks::null_sink_st;
#endif

  // this adapts an existing spdlog sink to cause it to stop repeating messages after the nth occurrence of the same message in a row.
  template <typename mutex, typename base, typename ... T> struct squelched_sink : public spdlog::sinks::base_sink<mutex> {

  public:
    squelched_sink(uint64_t limit, T... args)
      : impl(args...), limit(limit), last_exists(false), repetitions(0) {}
    virtual ~squelched_sink() {}
    virtual void _sink_it(const spdlog::details::log_msg& msg) {
      if (last_exists &&
        *last_logger_name == *msg.logger_name &&
        last_level == msg.level &&
        last_thread_id == msg.thread_id &&
        !strncmp(last_msg_buffer, msg.raw.c_str(), sizeof(last_msg_buffer))
        ) {
        uint64_t n = ++repetitions;
        if (n < limit) {
          impl.log(msg);
        } else if (n == limit) {
          spdlog::details::log_msg ellipsis(msg.logger_name, msg.level);
          ellipsis.thread_id = msg.thread_id;
          ellipsis.raw << msg.raw.c_str();
          ellipsis.formatted << msg.formatted.c_str() << "...\n"; // todo: drop preceding \n
          ellipsis.time = msg.time;
          impl.log(ellipsis);
        }
      } else {
        repetitions = 0;
        impl.log(msg);
      }

      last_exists = true;
      last_level = msg.level;
      last_logger_name = msg.logger_name;
      last_thread_id = msg.thread_id;
      strcpy(last_msg_buffer, msg.raw.c_str());
    }

    virtual void flush() {
      impl.flush();
    }

    bool last_exists;
    spdlog::level::level_enum last_level;
    const std::string * last_logger_name;
    spdlog::log_clock::time_point last_time;
    size_t last_thread_id;
    char last_msg_buffer[2048];
    uint64_t repetitions, limit;
    base impl;
  };

  // "template-typedefs"
  template <typename ... T> using squelched_sink_mt = squelched_sink<std::mutex, T...>;
  template <typename ... T> using squelched_sink_st = squelched_sink<spdlog::details::null_mutex, T...>;

  static inline shared_ptr<logger> log(const char * name) {
    auto result = spdlog::get(name);
    if (!result)
      result = spdlog::create<default_sink>(name);
    return result;
  }
}