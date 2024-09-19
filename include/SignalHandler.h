#pragma once

#include <atomic>
#include <condition_variable>
#include <future>
#include <mutex>
#include <Windows.h>

class Signal final {
public:
  static Signal instance;

  void SetSignal();

  [[nodiscard]] bool Signalled() const { return signaled; }

  void Wait();

private:
  std::atomic_bool signaled;
  std::condition_variable cv;
  std::mutex mutex;
};

class SignalHandler final {
public:
  explicit SignalHandler(bool runAsService) : runAsService(runAsService) {}

  void Start(HANDLE device);

  void Wait() const;

private:
  bool runAsService;
  std::future<void> task;

  void Handler(HANDLE device) const;
};
