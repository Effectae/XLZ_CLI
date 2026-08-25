#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <variant>
#include <xlz_cli/args.hpp>
#include <xlz_cli/core/parse_impl.hpp>
#include <xlz_cli/generic/generator.hpp>

using namespace XLZ_CLI;

using Needs = Generic::Needs;
using Convter = Generic::Converter;

namespace Cli {
auto list_options() -> void;

using ArgOpt = Generic::GenOpt<"arg", "Print if give a argument", std::optional<std::string_view>,
                               Needs::Once>;
using FloatOpt =
    Generic::GenOpt<"float", "Print if give a float", std::optional<double>, Needs::Once>;
using NumOpt = Generic::GenOpt<"num", "Print if give a num", std::size_t, Needs::Once,
                               Generic::converter<std::size_t>>;
using FlagOpt = Generic::GenOpt<"flag", "Print true if used flag", Generic::Flag, Needs::None>;
using OptSet =
    Generic::OptSet<ArgOpt, FloatOpt, NumOpt, FlagOpt,
                    Generic::GenOpt<"do-list", "List the options", std::monostate, Needs::None,
                                    [](Convter::Arg arg, Convter::ValPtr val) -> Convter::Report {
                                      std::println("Print the list of options:");
                                      list_options();
                                      std::println("The end of list.");
                                      return {};
                                    }>>;

constexpr static auto matcher = OptSet::gen_static_matcher();
constexpr static OptSet default_cli{{}, {}, std::numeric_limits<std::size_t>::max(), {}, {}};

auto list_options() -> void {
  constexpr auto size = OptSet::sum;

  const auto& names = matcher.names();
  const auto& descs = matcher.get_arrays<3>();

  for (std::size_t i{}; i < size; i++) std::println("\t{:<10} {}", names[i], descs[i]);
}
};  // namespace Cli

template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

auto main(int argc, char const* const* argv) -> int {
  Args<> args{argc, argv};

  auto cli = Cli::default_cli;  // Copy from the default value done on compile-time
  auto [it, result] = cli.parse<Cli::matcher>(++args.begin(), args.end());

  using Result = decltype(result);

  std::print("The state of parse:");
  std::visit(Overloaded{[](Result::Success) -> void { std::println("Success"); },
                        [](Result::ConvertErr const& err) -> void {
                          std::println("ConvertErr {}", err.err);
                        },
                        [&](Result::UnknownOption const& e) -> void {
                          auto opt = e.option;
                          opt.remove_prefix(2);
                          if (opt.empty())
                              // Support '--'
                              [[likely]] {
                            std::println("Success");
                            it++;
                            return;
                          };
                          std::println("Unknown option: {}", opt);
                        },
                        [](Result::MissingArgument const& e) -> void {
                          std::println("Missing an argument in{}", e.option);
                        },
                        [](Result::MissingArguments const& e) -> void {
                          std::println("Missing arguments in {}", e.option);
                        }},
             result.state);

  auto const& arg = cli.get<Cli::ArgOpt>().get();
  if (arg) std::println("Arg:{}", *arg);
  auto const& float_pointer = cli.get<Cli::FloatOpt>().get();
  if (float_pointer) std::println("Float:{}", *float_pointer);
  auto const& num = cli.get<Cli::NumOpt>().get();
  if (num != ~std::size_t{0}) std::println("Num:{}", num);
  auto const& flag = cli.get<Cli::FlagOpt>().get();
  std::println("flag : {}", flag ? "enabled" : "disabled");

  if (it != args.end()) {
    std::println("Extra arguments:");
    do {
      std::println("{}", *it);
    } while (++it != args.end());
  }
}
