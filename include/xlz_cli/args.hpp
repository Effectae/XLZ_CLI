#pragma once

#include <span>

namespace XLZ_CLI {
template <typename ArgStr = char const* const>
struct Args : std::span<ArgStr> {
  constexpr Args(int argc, ArgStr const* argv)
      : std::span<ArgStr>{argv, static_cast<std::size_t>(argc)} {}
};
}  // namespace XLZ_CLI
