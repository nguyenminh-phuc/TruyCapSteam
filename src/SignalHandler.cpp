#include "SignalHandler.h"
#include <stdexcept>
#include "aixlog_wrapper.hpp"
#include "windivert.h"
#include "Localizer.h"
#include "Utils.h"

Signal Signal::instance;

static BOOL WINAPI SignalCatcher(DWORD) {
  Signal::instance.SetSignal();
  return TRUE;
}

void Signal::SetSignal() {
  std::unique_lock lock(mutex);
  signaled = true;
  cv.notify_one();
}

void Signal::Wait() {
  std::unique_lock lock(mutex);
  cv.wait(lock, [this] { return signaled == true; });
}

void SignalHandler::Start(HANDLE device) {
  task = std::async(std::launch::async, [this, device] { Handler(device); });
}

void SignalHandler::Wait() const { task.wait(); }

void SignalHandler::Handler(HANDLE device) const {
  if (!runAsService)
    if (!SetConsoleCtrlHandler(SignalCatcher, TRUE)) {
      LOG(ERROR) << Utils::FormatError(FormatSetConsoleCtrlHandler) << '\n';
      throw std::runtime_error("This should never happen");
    }

  Signal::instance.Wait();

  LOG(INFO) << Localizer::Get(TxtAppStopping) << '\n';

  WinDivertShutdown(device, WINDIVERT_SHUTDOWN_BOTH);

  if (!runAsService)
    SetConsoleCtrlHandler(nullptr, FALSE);
}
