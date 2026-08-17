#pragma once

#include "./parse.hpp"

namespace XLZ_CLI::Core::Parse {
template <OptionMatcher Matcher, ValPtrsViewer ValPtrs>
[[gnu::noinline]] constexpr auto parse(auto args_begin, auto args_end,
                                       Matcher const& option_matcher, ValPtrs valptrs)
    -> std::pair<decltype(args_begin), Result> {
  using State = Result::State;

  auto i{args_begin};
  while (i != args_end) {
    std::string_view str_sv{*i};
    if (!option_matcher.is_option(str_sv)) [[unlikely]]
      break;
  _reback:
    if (auto it = option_matcher.find(str_sv); it) {
      auto idx{*it};
      auto val_ptr{valptrs[idx]};
      auto converter{option_matcher.get_converter(idx)};
      OptNeeds needs{option_matcher.get_needs(idx)};

      switch (needs) {
        case OptNeeds::None:
          if (auto report = converter({}, val_ptr); !report) [[unlikely]]
            return {i, std::move(report.error())};
          break;

        case OptNeeds::Once:
          if (++i == args_end) [[unlikely]]
            goto _err_missing_argument;
          if (auto opt = *i; option_matcher.is_option(opt)) [[unlikely]]
            goto _err_missing_argument;
          else
            str_sv = opt;
          if (auto report = converter(str_sv, val_ptr); !report) [[unlikely]]
            return {i, std::move(report.error())};
          break;
        _err_missing_argument:
          return {i, State{Result::MissingArgument{str_sv}}};

        case OptNeeds::Multi:
          while (true) {
            if (++i == args_end) [[unlikely]]
              goto _end;
            str_sv = *i;
            if (option_matcher.is_option(str_sv)) [[unlikely]]
              goto _reback;
            if (auto report = converter(str_sv, val_ptr); !report) [[unlikely]]
              return {i, std::move(report.error())};
          }

        default:
          std::unreachable();
      }
    } else [[unlikely]]
      return {i, State{Result::UnknownOption{str_sv}}};
    ++i;
  }
_end:
  return {i, {}};
}
}  // namespace XLZ_CLI::Core::Parse
