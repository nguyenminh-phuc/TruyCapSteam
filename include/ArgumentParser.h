#pragma once

#include <stdint.h>
#include <optional>
#include <memory>
#include <set>
#include <string>

enum class ParseResultType {
  Run,
  Install,
  Uninstall,
  ShowHelp,
  ShowCompletion,
  InvalidArgument
};

struct ParseResult final {
  ParseResultType type{};
  std::optional<std::string> explanatoryString;
  bool hasArguments{};
};

struct ParsedArguments final {
  std::set<uint16_t> ports;
  uint16_t windowSize{};
  bool runAsService{};

  static std::unique_ptr<ParsedArguments> instance;
};

class ArgumentParser final {
public:
  static ParseResult Parse(int argc, char **argv);

  ArgumentParser() = delete;
};
