#include <array>
#include <cmath>
#include <cstddef>
#include <print>
#include <ranges>
#include <variant>
#include <xlz_cli/args.hpp>
#include <xlz_cli/core/parse_impl.hpp>
#include <xlz_cli/generic/generator.hpp>

using namespace XLZ_CLI;

using Needs = XLZ_CLI::Core::Parse::OptNeeds;
using Convter = Generic::Converter;

auto help() -> void;

using NumOpt =
    Generic::GenOpt<"Num", std::size_t, Needs::Once, Generic::converter<std::size_t, 10>>;
constexpr auto options =
    Generic::OptSet<Generic::GenOpt<"help", std::monostate, Needs::None,
                                    [](Convter::Arg arg, Convter::ValPtr val) -> Convter::Report {
                                      std::println("Print the list of help:");
                                      help();
                                      return {};
                                    }>,
                    NumOpt>{std::monostate{}, ~std::size_t{0}};  // Set default value

auto help() -> void {
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
  auto runtime_matcher = matcher.make_base();
  std::array<Convter::ValPtr, decltype(options)::sum> vals;
  cli.bind<matcher>(vals);
  auto [it, result] =
      Core::Parse::parse(++args.begin(), args.end(), runtime_matcher, std::span{vals});

  using Result = decltype(result);

  std::print("The state of parse:");
  std::visit(Overloaded{[](Result::Success) -> void { std::println("Success"); },
                        [](Result::ConvertErr const& err) -> void {
                          std::println("ConvertErr {}", err.err);
                        },
                        [&](Result::UnknownOption const& e) -> void {
                          auto opt = e.option;
                          opt = opt.substr(2);
                          if (opt.empty())
                              // Support '--'
                              [[likely]] {
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

  auto const& num = static_cast<NumOpt&>(cli);
  if (auto v = num.value; v != ~std::size_t{0}) std::println("Num {}", v);

  if (it != args.end()) {
    std::println("Extra arguments:");
    do {
      std::println("{}", *it);
    } while (++it != args.end());
  }
}
