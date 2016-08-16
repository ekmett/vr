#pragma once

#include <type_traits>
#include <functional>
#include <stdexcept>

namespace framework {
  namespace serialization {
    struct serialization_error : std::runtime_error {
      template <typename ... Ts> serialization_error(Ts &&... args) : runtime_error(std::forward(args)...) {}    
    };

    template <typename stream> checked_stream {
      typedef stream::is_reading is_reading;
      typedef stream::is_writing is_writing;

      stream base;

      template <typename ... Ts> checked_stream(T && args) : base(std::forward(args)...) {}

      template <typename T> void range(T value, const std::remove_reference<T> min, const std::remove_reference<T> max) {
        assert(min < max);
        if (stream::is_writing) {
          assert(min <= value);
          assert(value <= max);
        }
        base.range(value, min, max);
        if (stream::is_reading) {
          assert(min <= value);
          assert(value <= max);
        }
      }

      template <typename T> void bits(Stream s, T value, int bits) {
        assert(bits > 0);
        assert(bits <= 32);
        base.bits(value, bits);
      }
    }
  }
}