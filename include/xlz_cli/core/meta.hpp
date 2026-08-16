#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <utility>

#include "./parse.hpp"

namespace XLZ_CLI::Core::Meta {
template <class Opt, std::size_t N>
struct SortedOpts : std::array<Opt, N> {
  template <class C>
  constexpr SortedOpts(std::array<Opt, N> arr, C&& cmp)
      : std::array<Opt, N>{[&]() constexpr -> std::array<Opt, N> {
          return (std::sort(arr.begin(), arr.end(), std::forward<C>(cmp)), arr);
        }()} {}
};

template <std::size_t Sum, std::size_t NameIdx, std::size_t ConverterIdx, std::size_t NeedsIdx,
          class... Ts>
struct StaticOptionMatcher {
  std::tuple<std::array<Ts, Sum>...> inner;
  constexpr static auto sum = Sum;

  template <std::size_t N>
  [[nodiscard]]
  constexpr auto get_arrays() const -> std::array<decltype(std::get<N>(inner)[0]), sum> const& {
    return std::get<N>(inner);
  }

  [[nodiscard]]
  constexpr auto names() const -> decltype(get_arrays<NameIdx>()) const& {
    return get_arrays<NameIdx>();
  }
  [[nodiscard]]
  constexpr auto converters() const -> decltype(get_arrays<ConverterIdx>()) const& {
    return get_arrays<ConverterIdx>();
  }
  [[nodiscard]]
  constexpr auto needs() const -> decltype(get_arrays<NeedsIdx>()) const& {
    return get_arrays<NeedsIdx>();
  }

  [[nodiscard]]
  consteval auto find(std::string_view opt) const -> std::size_t {
    auto const& names = std::get<NameIdx>(inner);
    auto b{names.begin()};
    auto e{names.end()};
    auto it = std::lower_bound(b, e, opt);
    if (it == e || *it != opt) return ~std::size_t{0};
    return std::distance(b, it);
  }

  template <class Opt, class Fn>
  constexpr static auto make_matcher(SortedOpts<Opt, Sum> const& arr, Fn&& to_tuple)
      -> StaticOptionMatcher {
    std::tuple<std::array<Ts, Sum>...> ret;

    [&]<std::size_t... Xs>(std::index_sequence<Xs...>) constexpr -> void {
      (
          [&](auto I) constexpr -> void {
            auto tuple{std::forward<Fn>(to_tuple)(arr[I])};
            [&]<std::size_t... Is>(std::index_sequence<Is...>) constexpr -> void {
              ((std::get<Is>(ret)[I] = std::get<Is>(tuple)), ...);
            }(std::index_sequence_for<Ts...>{});
          }(Xs),
          ...);
    }(std::make_index_sequence<Sum>{});

    return {ret};
  }

  [[nodiscard]]
  constexpr auto make_base() const -> XLZ_CLI::Core::Parse::Base::OptionMatcher {
    XLZ_CLI::Core::Parse::Base::OptionMatcher ret{.opt_names = std::get<NameIdx>(inner),
                                                  .converters = std::get<ConverterIdx>(inner),
                                                  .needs_arr = std::get<NeedsIdx>(inner)};
    return ret;
  }
};
}  // namespace XLZ_CLI::Core::Meta
