# XLZ_CLI

## Overview

XLZ_CLI maps command-line options to C++ objects without runtime string hashing or virtual calls. Option definitions are processed at compile time into sorted arrays and static indices. The runtime parser consumes arguments as a linear pass, converting values via std::from_chars and reporting errors through std::expected.

## Core Design

- Compile-time sorting – option names are sorted during constant evaluation using `constexpr std::sort`. The parser uses binary search at runtime.

- Static indexing - `consteval` member `find` resolves option names to array indices during compilation.

- Memory layout - option storage is plain `std::array` of `void*` pointers. No heap allocations occur during parsing unless the user supplies a `std::vector` multi-value option.

- Separation of concerns - the static matcher (`StaticOptionMatcher`) holds compile-time arrays. A lightweight runtime view (`Base::OptionMatcher`) wraps `std::span` for the parser.

## Quick Example

See [example.cpp](tests/example.cpp).

### Build with example

```sh
$ meson setup build -Dtests=true && meson compile -C build
$ ./build/tests/example --do-list --num 0x10 --arg 'This is a argument' --float -inf --flag
Print the list of options:
        arg        Print if give a argument
        do-list    List the options
        flag       Print true if used flag
        float      Print if give a float
        num        Print if give a num
The end of list.
The state of parse:Success
Arg:This is a argument
Float:-inf
Num:16
flag : enabled
```

## Defining Options

```cpp
using MyOpt = Generic::GenOpt<"name","This is description.",ValueType,OptNeeds,Converter>;
```

- `OptNeeds`:`None`(no argument), `Once`(exactly one), `Multi`(zero or more).

- `Converter` default to `converter<ValueType>`;you can provide a custom callable.

## Built-in Converters

| Type | Behaviour |
|:----:|:---------:|
| std::string_view | stores view directly |
| integrals (`int`,`size_t`,....) | `std::from_chars` with optional base |
| floating-point | `std::from_chars` |
| `std::optional` | forwards to `T`, `emplace` on match |
| `std::vector<T>` | forwards to `T`, appends each occurrence |
| `Flag` | set `inner = true`, ignores argument |
| `std::monostate` | action only, no storage |

Add your own by overloading `converter`.

## Error Handling

Conversion errors return `std::expected<std::monostate,std::string>`.
Parse errors are captured in a `std::variant` of:

- `Success`
- `ConvertErr`
- `UnknownOption`
- `MissingArgument`
- `MissingArguments`

## Requirements

- c++23 compiler (GCC 13+, Clang 17+)
- Standard library features: `std::print`, `std::expected`, `std::from_chars`, `constexpr std::sort`
- No external dependencies

## Acknowledgements

This project would not exist without the incredible tools provided by modern C++.

- **`consteval`** – forces option name lookup to happen at compile time.
- **`constexpr std::sort`** – enables compile-time sorting of option names.
- **`std::expected`** – makes error handling clean and exception-free.
- **`std::print`** – offers type-safe, fast output.
- **`std::from_chars`** – provides blazing fast numeric conversion.

Thank you to the C++ community and standard committee for pushing the language forward.

## Warning

> **⚠️ Experimental Project**
> This is a personal experiment to explore C++23 compile-time metaprogramming and zero-overhead CLI parsing.
> **Not production-ready.** APIs may change, edge cases may be rough, and documentation is minimal.
> Use it for learning and inspiration, but not for mission-critical systems.

## License

MIT License - see [LICENSE](LICENSE) file for details.
