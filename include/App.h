#pragma once

#include <stdint.h>
#include <memory>
#include <set>
#include <Windows.h>
#include <string>
#include "ArgumentParser.h"
#include "SignalHandler.h"

class App final {
public:
  static std::unique_ptr<App> Initialize(const ParsedArguments &args);

  void Run();

private:
  bool runAsService;
  std::set<uint16_t> ports;
  uint16_t windowSize;
  std::string filter;
  SignalHandler signalHandler;
  HANDLE device;

  explicit App(const ParsedArguments &args);

  void LogRunningMessage() const;
};
