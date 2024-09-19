#pragma once

#include <optional>
#include <string>
#include <Windows.h>
#include "Localizer.h"

class Utils final {
public:
  // std::runtime_format is only supported in C++26 :(
  static std::string Format(const char *format, ...);

  template <typename... Args>
  static std::string Format(LocalizationKey format, Args... args) {
    return Format(Localizer::Get(format).c_str(), args...);
  }

  static std::string GetSystemError(DWORD error);

  static std::string FormatError(LocalizationKey format,
                                 bool getLocalizedError = false,
                                 std::optional<DWORD> error = {});

  static void SetUtf8();

  static std::string Utf8Encode(const std::wstring &wstr);

  static std::wstring Utf8Decode(const std::string &str);

  Utils() = delete;
};
