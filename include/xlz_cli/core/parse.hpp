#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace XLZ_CLI::Core::Parse {
struct Result {
  struct Success {};
  struct ConvertErr {
    std::string err;
  };
  struct UnknownOption {
    std::string_view option;
  };
  struct MissingArgument {
    std::string_view option;
  };
  struct MissingArguments {
    std::string_view option;
  };

  using State = std::variant<Success, ConvertErr, UnknownOption, MissingArgument, MissingArguments>;
  State state;

  constexpr Result() : state{Success{}} {}
  constexpr Result(State&& s) : state{s} {}
  constexpr Result(std::string&& err) : state{ConvertErr{err}} {}
};

enum class OptNeeds : std::uint8_t {
  None = 0,
  Once,
  Multi,
};

struct Converter {
  using Arg = std::string_view;
  using ValPtr = void*;
  using Report = std::expected<std::monostate, std::string>;

  using Func = Report(Arg, ValPtr);
  using FuncPtr = Func*;

  FuncPtr convert_func;

  constexpr Converter() = default;

  template <class F>
  constexpr Converter(F f) : convert_func{f} {}

  constexpr auto operator()(Arg arg, ValPtr val) const -> Report { return convert_func(arg, val); }
};

template <class T>
concept OptionMatcher = requires(T t) {
  { t.is_option(std::string_view{}) } -> std::same_as<bool>;
  { t.find(std::string_view{}) } -> std::same_as<std::optional<std::size_t> >;
  { t.get_converter(std::size_t{}) } -> std::convertible_to<Converter>;
  { t.get_needs(std::size_t{}) } -> std::convertible_to<OptNeeds>;
};

template <class T>
concept ValPtrsViewer = requires(T v, std::size_t idx) {
  { v[idx] } -> std::convertible_to<Converter::ValPtr>;
};

namespace Base {
using ValPtrsViewer = std::span<Converter::ValPtr>;

struct OptionMatcher {
  std::span<std::string_view const> opt_names;
  std::span<Converter const> converters;
  std::span<OptNeeds const> needs_arr;

  [[nodiscard]]
  static constexpr auto is_option(std::string_view const& arg) -> bool {
    return arg.starts_with("--");
  }
  [[nodiscard]]
  constexpr auto find(std::string_view opt) const -> std::optional<std::size_t> {
    auto b{opt_names.begin()};
    auto e{opt_names.end()};
    opt.remove_prefix(2);
    auto it = std::lower_bound(b, e, opt);
    if (it == e || *it != opt) return std::nullopt;
    return std::distance(b, it);
  }
  [[nodiscard]]
  constexpr auto get_converter(std::size_t idx) const -> Converter const& {
    return converters[idx];
  }
  [[nodiscard]]
  constexpr auto get_needs(std::size_t idx) const -> OptNeeds const& {
    return needs_arr[idx];
  }
};
};  // namespace Base

template <OptionMatcher Matcher, ValPtrsViewer ValPtrs>
constexpr auto parse(auto args_begin, auto args_end, Matcher const& option_matcher, ValPtrs valptrs)
    -> std::pair<decltype(args_begin), Result>;
}  // namespace XLZ_CLI::Core::Parse
