#include "ServiceServer.h"
#include <assert.h>
#include <exception>
#include "aixlog_wrapper.hpp"
#include "App.h"
#include "ArgumentParser.h"
#include "Localizer.h"
#include "Logger.h"
#include "SignalHandler.h"
#include "Utils.h"

ServiceServer ServiceServer::instance;

int ServiceServer::Run() {
  const auto serviceName = Utils::Utf8Decode(myServiceName);
  const SERVICE_TABLE_ENTRY dispatchTable[] = {
      {const_cast<LPWSTR>(serviceName.c_str()), Main}, {nullptr, nullptr}};

  if (!StartServiceCtrlDispatcher(dispatchTable)) {
    LOG(FATAL) << Utils::FormatError(FormatStartServiceCtrlDispatcher) << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void ServiceServer::Main(DWORD, LPWSTR *) {
  assert(ParsedArguments::instance);

  try {
    const auto serviceName = Utils::Utf8Decode(myServiceName);
    AixLog::Log::init<SinkEventLog>(serviceName, AixLog::Severity::trace);

    instance.statusHandle =
        RegisterServiceCtrlHandler(serviceName.c_str(), &Handle);
    if (!instance.statusHandle) {
      LOG(FATAL) << Utils::FormatError(FormatRegisterServiceCtrlHandler)
                 << '\n';
      return;
    }

    {
      std::lock_guard lock(instance.statusMutex);
      instance.status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
      instance.status.dwCurrentState = SERVICE_START_PENDING;
      SetServiceStatus(instance.statusHandle, &instance.status);
    }

    const auto app = App::Initialize(*ParsedArguments::instance);
    if (!app) {
      instance.SetErrorStatus();
      return;
    }

    {
      std::lock_guard lock(instance.statusMutex);
      instance.status.dwCurrentState = SERVICE_RUNNING;
      instance.status.dwControlsAccepted =
          SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
      SetServiceStatus(instance.statusHandle, &instance.status);
    }

    app->Run();

    {
      std::lock_guard lock(instance.statusMutex);
      instance.status.dwCurrentState = SERVICE_STOPPED;
      SetServiceStatus(instance.statusHandle, &instance.status);
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what() << '\n';
    instance.SetErrorStatus();
  }
}

void ServiceServer::Handle(DWORD control) {
  if (control != SERVICE_CONTROL_STOP && control != SERVICE_CONTROL_SHUTDOWN)
    return;

  {
    std::lock_guard lock(instance.statusMutex);
    if (instance.status.dwCurrentState != SERVICE_RUNNING)
      return;

    instance.status.dwCurrentState = SERVICE_STOP_PENDING;
    instance.status.dwControlsAccepted = 0;
    SetServiceStatus(instance.statusHandle, &instance.status);
  }

  Signal::instance.SetSignal();
}

void ServiceServer::SetErrorStatus(DWORD error) {
  std::lock_guard lock(instance.statusMutex);
  status.dwCurrentState = SERVICE_STOPPED;
  status.dwControlsAccepted = 0;
  status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
  status.dwServiceSpecificExitCode = error;
  SetServiceStatus(statusHandle, &status);
}
