#pragma once

#include <Windows.h>
#include "aixlog_wrapper.hpp"

class SinkConsole final : public AixLog::Sink {
public:
  explicit SinkConsole(const AixLog::Filter &filter) : Sink(filter) {}

  void log(const AixLog::Metadata &metadata,
           const std::string &message) override;
};

class SinkEventLog final : public AixLog::Sink {
public:
  // aixlog syncs a new log every time it hits a newline '\n'
  // so to make sure a message with '\n' is reported as one event
  // instead of split into multiple ones, we use ';'
  static constexpr char nl = ';';
  static void ReplaceNewLine(std::string &message);

  SinkEventLog(const std::wstring &ident, const AixLog::Filter &filter);

  void log(const AixLog::Metadata &metadata,
           const std::string &message) override;

  ~SinkEventLog() override;

protected:
  HANDLE eventLog;

  static WORD GetEventType(AixLog::Severity severity);
};
