#include "Logger.h"
#include <algorithm>
#include <format>
#include <iostream>
#include <print>
#include "Localizer.h"
#include "Utils.h"

static std::string BuildLog(const AixLog::Metadata &metadata,
                            const std::string &message) {
  switch (metadata.severity) {
  case AixLog::Severity::trace:
  case AixLog::Severity::debug:
  case AixLog::Severity::info:
  case AixLog::Severity::notice: {
    if (metadata.tag.text == "default")
      return message;

    std::stringstream ss;
    ss << std::format("[{}] {}", to_string(metadata.severity), message);
    return ss.str();
  }
  case AixLog::Severity::warning:
  case AixLog::Severity::error:
  case AixLog::Severity::fatal:
  default: {
    if (metadata.tag.text == "default")
      return message;

    std::stringstream ss;
    ss << std::format("[{}] {}", to_string(metadata.severity), message);
    if (metadata.timestamp)
      ss << std::format("\n\ttime: {}", metadata.timestamp.to_string());
    if (metadata.function) {
      ss << std::format("\n\tfunc: {}\n", metadata.function.name);
      ss << std::format("\tline: {}\n", metadata.function.line);
      ss << std::format("\tfile: {}", metadata.function.file);
    }
    return ss.str();
  }
  }
}

void SinkConsole::log(const AixLog::Metadata &metadata,
                      const std::string &message) {
  const auto log = BuildLog(metadata, message);

  switch (metadata.severity) {
  case AixLog::Severity::trace:
  case AixLog::Severity::debug:
  case AixLog::Severity::info:
  case AixLog::Severity::notice: {
    std::println("{}", log);
    std::cout.flush();
    break;
  }
  case AixLog::Severity::warning:
  case AixLog::Severity::error:
  case AixLog::Severity::fatal: {
    std::println(std::cerr, "{}", log);
    break;
  }
  }
}

void SinkEventLog::ReplaceNewLine(std::string &message) {
  std::ranges::replace(message, '\n', nl);
}

SinkEventLog::SinkEventLog(const std::wstring &ident,
                           const AixLog::Filter &filter)
    : Sink(filter) {
  eventLog = RegisterEventSource(nullptr, ident.c_str());

  if (!eventLog)
    LOG(ERROR) << Utils::FormatError(FormatRegisterEventSource) << '\n';
}

void SinkEventLog::log(const AixLog::Metadata &metadata,
                       const std::string &message) {
  if (!eventLog)
    return;

  const auto type = GetEventType(metadata.severity);
  const auto log = BuildLog(metadata, message);
  const auto wideLog = Utils::Utf8Decode(log);
  auto wideLogPtr = wideLog.c_str();

  ReportEvent(eventLog, type, 0, 0, nullptr, 1, 0, &wideLogPtr, nullptr);
}

SinkEventLog::~SinkEventLog() {
  if (eventLog)
    DeregisterEventSource(eventLog);
}

WORD SinkEventLog::GetEventType(AixLog::Severity severity) {
  // https://msdn.microsoft.com/de-de/library/windows/desktop/aa363679(v=vs.85).aspx
  switch (severity) {
  case AixLog::Severity::trace:
  case AixLog::Severity::debug:
    return EVENTLOG_INFORMATION_TYPE;
  case AixLog::Severity::info:
  case AixLog::Severity::notice:
    return EVENTLOG_SUCCESS;
  case AixLog::Severity::warning:
    return EVENTLOG_WARNING_TYPE;
  case AixLog::Severity::error:
  case AixLog::Severity::fatal:
    return EVENTLOG_ERROR_TYPE;
  default:
    return EVENTLOG_INFORMATION_TYPE;
  }
}
