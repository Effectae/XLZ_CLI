#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <format>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "../core/meta.hpp"
#include "../core/parse.hpp"

namespace XLZ_CLI::Generic {
using Converter = XLZ_CLI::Core::Parse::Converter;

template <typename T>
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report;

template <>
constexpr auto converter<std::string_view>(Converter::Arg arg, Converter::ValPtr val)
    -> Converter::Report {
  *static_cast<std::string_view*>(val) = arg;
  return {};
}

template <std::integral N, int base>
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  N* n = static_cast<N*>(val);
  auto [p, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), *n, base);
  if (ec != decltype(ec){}) [[unlikely]]
    return std::unexpected{std::make_error_code(ec).message()};
  if (*p) [[unlikely]]
    return std::unexpected{std::format("Failed : It has extra chars \"{}\"", p)};
  return {};
};

template <std::floating_point N>
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  N* n = static_cast<N*>(val);
  auto [p, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), *n);
  if (ec != decltype(ec){}) [[unlikely]]
    return std::unexpected{std::make_error_code(ec).message()};
  if (*p) [[unlikely]]
    return std::unexpected{std::format("Failed : It has extra chars \"{}\"", p)};
  return {};
};

// optional<T>
template <typename T>
  requires std::is_same_v<T, std::optional<typename T::value_type>>
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  return converter<typename T::value_type>(
      arg, static_cast<Converter::ValPtr>(&((*static_cast<T*>(val)).emplace())));
}

// optional<INTEGRAL,base>
template <typename T, int base>
  requires std::is_same_v<T, std::optional<typename T::value_type>> &&
           std::is_integral_v<typename T::value_type>
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  return converter<typename T::value_type, base>()(
      arg, static_cast<Converter::ValPtr>(&((*static_cast<T*>(val)).emplace())));
}
// vector<T>
template <typename T>
  requires std::is_same_v<T, std::vector<typename T::value_type>>
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  return converter<typename T::value_type>()(
      arg, static_cast<Converter::ValPtr>(&((*static_cast<T*>(val)).emplace_back())));
}
// vector<INTEGRAL,base>
template <typename T, int base>
  requires std::is_same_v<T, std::vector<typename T::value_type>> &&
           requires { typename T::value_type; }
constexpr auto converter(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  return converter<typename T::value_type, base>()(
      arg, static_cast<Converter::ValPtr>(&((*static_cast<T*>(val)).emplace_back())));
}

struct Flag {
  bool inner{};
};

template <>
constexpr auto converter<Flag>(Converter::Arg arg, Converter::ValPtr val) -> Converter::Report {
  static_cast<Flag*>(val)->inner = true;
  return {};
};

template <std::size_t N>
struct FixedString {
  std::array<char, N> storage;
  constexpr FixedString(const char (&str)[N]) : storage{std::to_array(str)} {}
  constexpr operator std::string_view() const { return {storage.data(), storage.size() - 1}; }
};

template <FixedString Name, class Vt, XLZ_CLI::Core::Parse::OptNeeds Needs,
          Converter F = converter<Vt>>
struct GenOpt {
  using value_type = Vt;
  static constexpr Converter func{F};
  static constexpr std::string_view name{Name};
  static constexpr XLZ_CLI::Core::Parse::OptNeeds needs{Needs};
  [[no_unique_address]]
  Vt value;

  constexpr explicit operator Vt&() { return value; };
  constexpr explicit operator Vt const&() const { return value; };
  [[nodiscard]]
  constexpr auto get() const -> Vt const& {
    return value;
  };
  [[nodiscard]]
  constexpr auto get() -> Vt& {
    return value;
  };

  constexpr GenOpt() : value{} {}

  constexpr GenOpt(Vt val) : value{val} {}
};

template <class T>
concept Option = requires(T t) {
  typename T::value_type;
  { t.func } -> std::convertible_to<Converter>;
  { t.name } -> std::convertible_to<std::string_view>;
  { t.needs } -> std::convertible_to<XLZ_CLI::Core::Parse::OptNeeds>;
};

template <Option... opts>
struct OptSet : opts... {
  struct Opt {
    Converter func;
    std::string_view name;
    XLZ_CLI::Core::Parse::OptNeeds needs;
  };
  static constexpr auto sum = sizeof...(opts);
  static constexpr XLZ_CLI::Core::Meta::SortedOpts<Opt, sum> sorted_opts{
      []() constexpr -> std::array<Opt, sum> {
        return {Opt{.func{opts::func}, .name{opts::name}, .needs = opts::needs}...};
      }(),
      [](Opt const& a, Opt const& b) constexpr -> bool { return a.name < b.name; }};

  constexpr OptSet() = default;

  constexpr OptSet(opts::value_type... def_vals) : opts{def_vals}... {}

  using StaticMatcher =
      XLZ_CLI::Core::Meta::StaticOptionMatcher<sum, 0, 1, 2, std::string_view, Converter,
                                               XLZ_CLI::Core::Parse::OptNeeds>;
  [[nodiscard]]
  constexpr auto gen_static_matcher() const -> StaticMatcher {
    return StaticMatcher::make_matcher(
        sorted_opts,
        [](Opt const& a) constexpr
            -> std::tuple<std::string_view, Converter, XLZ_CLI::Core::Parse::OptNeeds> {
          return {a.name, a.func, a.needs};
        });
  }

  template <auto const& StaticOptionMatcher>
  constexpr auto bind(XLZ_CLI::Core::Parse::ValPtrsViewer auto& valptrs) -> void {
    (
        [&]() constexpr -> void {
          constexpr auto idx = StaticOptionMatcher.find(opts::name);
          static_assert(idx != ~std::size_t{0},
                        "One or more option names not found in static matcher");
          valptrs[idx] = reinterpret_cast<Converter::ValPtr>(&static_cast<opts*>(this)->get());
        }(),
        ...);
  }

  template <Option Opt>
  [[nodiscard]]
  constexpr auto get() -> Opt& {
    return *this;
  };
  template <Option Opt>
  [[nodiscard]]
  constexpr auto get() const -> Opt const& {
    return *this;
  };

  template <auto const& StaticOptionMatcher>
  auto parse(auto args_begin, auto args_end)
      -> std::pair<decltype(args_begin), XLZ_CLI::Core::Parse::Result> {
    std::array<Converter::ValPtr, sum> vals;
    bind<StaticOptionMatcher>(vals);
    std::span<Converter::ValPtr> vals_span{vals};
    static constexpr auto matcher = StaticOptionMatcher.make_base();
    return XLZ_CLI::Core::Parse::parse(args_begin, args_end, matcher, vals_span);
  }
};

}  // namespace XLZ_CLI::Generic
