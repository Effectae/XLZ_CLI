#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <xlz_cli/args.hpp>
#include <xlz_cli/core/parse_impl.hpp>
#include <xlz_cli/generic/generator.hpp>

using namespace XLZ_CLI;

using Needs = XLZ_CLI::Core::Parse::OptNeeds;
using Convter = Generic::Converter;

auto list_the_options() -> void;

using ArgOpt = Generic::GenOpt<"arg", std::optional<std::string_view>, Needs::Once>;
using FloatOpt = Generic::GenOpt<"float", std::optional<double>, Needs::Once>;
using NumOpt = Generic::GenOpt<"num", std::size_t, Needs::Once, Generic::converter<std::size_t>>;

constexpr auto options =
    Generic::OptSet<ArgOpt, FloatOpt, NumOpt,
                    Generic::GenOpt<"do-list", std::monostate, Needs::None,
                                    [](Convter::Arg arg, Convter::ValPtr val) -> Convter::Report {
                                      std::println("Print the list of options:");
                                      list_the_options();
                                      std::println("The end of list.");
                                      return {};
                                    }>>{
        {}, {}, std::numeric_limits<std::size_t>::max(), std::monostate{}};  // Set default value

auto list_the_options() -> void {
  for (auto const [index, option] : decltype(options)::sorted_opts | std::views::enumerate)
    std::println("[{}]:{}", index, option.name);
}

template <class... Ts>
struct Overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

auto main(int argc, char const* const* argv) -> int {
  Args<> args{argc, argv};

  constexpr static auto matcher = options.gen_static_matcher();
  auto cli = options;  // Copy from the default value done on compile-time
  auto [it, result] = cli.parse<matcher>(++args.begin(), args.end());

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

  auto const& arg = cli.get<ArgOpt>().get();
  if (arg) std::println("Arg:{}", *arg);
  auto const& float_pointer = cli.get<FloatOpt>().get();
  if (float_pointer) std::println("Float:{}", *float_pointer);
  auto const& num = cli.get<NumOpt>().get();
  if (num != ~std::size_t{0}) std::println("Num:{}", num);

  if (it != args.end()) {
    std::println("Extra arguments:");
    do {
      std::println("{}", *it);
    } while (++it != args.end());
  }
}
