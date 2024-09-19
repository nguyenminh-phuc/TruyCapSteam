#include "Utils.h"
#include <locale.h>
#include <system_error>
#include <Windows.h>
#include "aixlog_wrapper.hpp"

// https://stackoverflow.com/a/55425070
std::string Utils::Format(const char *format, ...) {
  std::string result;
  va_list args, args_copy;

  va_start(args, format);
  va_copy(args_copy, args);

  const int len = vsnprintf(nullptr, 0, format, args);
  if (len < 0) {
    va_end(args_copy);
    va_end(args);
    throw std::runtime_error("vsnprintf error");
  }

  if (len > 0) {
    result.resize(len);
    // note: &result[0] is *guaranteed* only in C++11 and later
    // to point to a buffer of contiguous memory with room for a
    // null-terminator, but this "works" in earlier versions
    // in *most* common implementations as well...
    vsnprintf(&result[0], len + 1, format,
              args_copy); // or result.data() in C++17 and later...
  }

  va_end(args_copy);
  va_end(args);

  return result;
}

std::string Utils::GetSystemError(DWORD error) {
  return std::system_category().message(static_cast<int>(error));
}

std::string Utils::FormatError(LocalizationKey format, bool getLocalizedError,
                               std::optional<DWORD> error) {
  const auto code = error ? *error : GetLastError();
  const auto errorMessage =
      getLocalizedError ? Localizer::GetError(code) : GetSystemError(code);
  auto fullMessage =
      Format(Localizer::Get(format).c_str(), code, errorMessage.c_str());

  return fullMessage;
}

// https://github.com/microsoft/STL/issues/4110#issuecomment-1773680119
void Utils::SetUtf8() {
  setlocale(LC_CTYPE, ".UTF8");
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
}

// https://stackoverflow.com/a/3999597
// Convert a wide Unicode string to an UTF8 string
std::string Utils::Utf8Encode(const std::wstring &wstr) {
  if (wstr.empty())
    return {};
  int size_needed =
      WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()),
                          nullptr, 0, nullptr, nullptr);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()),
                      &strTo[0], size_needed, nullptr, nullptr);
  return strTo;
}

// Convert an UTF8 string to a wide Unicode String
std::wstring Utils::Utf8Decode(const std::string &str) {
  if (str.empty())
    return {};
  int size_needed = MultiByteToWideChar(
      CP_UTF8, 0, &str[0], static_cast<int>(str.size()), nullptr, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()),
                      &wstrTo[0], size_needed);
  return wstrTo;
}
