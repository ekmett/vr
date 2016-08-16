#pragma once

#include <type_traits>

namespace framework {

  namespace detail {
    template<typename ... Fs>
    struct composition_impl {
      composition_impl(Fs&& ... fs) : functions(std::forward<Fs>(fs) ...) {}

      template<size_t N, typename ... Ts>
      auto apply(std::integral_constant<size_t, N>, Ts&& ... ts) const {
        return apply(std::integral_constant<size_t, N - 1>{}
        , std::get<N>(functions)(std::forward<Ts>(ts)...));
      }

      template<typename ... Ts>
      auto apply(std::integral_constant<size_t, 0>, Ts&& ... ts) const {
        return std::get<0>(functions)(std::forward<Ts>(ts)...);
      }

      template<typename ... Ts>
      auto operator()(Ts&& ... ts) const {
        return apply(std::integral_constant<size_t, sizeof ... (Fs)-1>{}, std::forward<Ts>(ts) ...);
      }

      std::tuple<Fs ...> functions;
    };
  }

  template<typename ... Fs>
  auto composition(Fs&& ... fs) {
    //return detail::composition_impl<Fs ...>(std::forward<Fs>(fs) ...);
    return detail::composition_impl<std::decay_t<Fs> ...>(std::forward<Fs>(fs) ...);
  }
}