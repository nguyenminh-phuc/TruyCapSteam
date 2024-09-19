#include "ServiceClient.h"
#include <assert.h>
#include <chrono>
#include <format>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <Windows.h>
#include "aixlog_wrapper.hpp"
#include "App.h"
#include "ArgumentParser.h"
#include "Localizer.h"
#include "ServiceServer.h"
#include "Utils.h"

using namespace std::chrono_literals;

struct Service final {
  SC_HANDLE handle;
  std::string name;
  std::wstring wideName;
};

static std::string FormatError(LocalizationKey format,
                               const std::string &serviceName,
                               std::optional<DWORD> error = {}) {
  const auto code = error ? *error : GetLastError();
  const auto errorMessage = Utils::GetSystemError(code);
  auto fullMessage =
      Utils::Format(format, serviceName.c_str(), code, errorMessage.c_str());

  return fullMessage;
}

static std::optional<SC_HANDLE> OpenManager() {
  const auto manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
  if (!manager) {
    LOG(ERROR) << Utils::FormatError(FormatOpenSCManager) << '\n';
    return {};
  }

  return manager;
}

static std::optional<SERVICE_STATUS_PROCESS> GetStatus(const Service &service) {
  SERVICE_STATUS_PROCESS status{};
  DWORD bytesNeeded{};
  if (!QueryServiceStatusEx(service.handle, SC_STATUS_PROCESS_INFO,
                            reinterpret_cast<LPBYTE>(&status),
                            sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded)) {
    LOG(ERROR) << FormatError(FormatQueryServiceStatusEx, service.name) << '\n';
    return {};
  }

  return status;
}

static std::optional<Service> CreateMySvc(SC_HANDLE manager,
                                          const wchar_t *servicePath) {
  std::wstringstream ss;
  ss << std::format(L"\"{}\"", servicePath);
  for (const auto port : ParsedArguments::instance->ports)
    ss << std::format(L" -p {}", port);
  ss << std::format(L" -w {}", ParsedArguments::instance->windowSize);
  ss << L" --run-as-service";
  const auto fullPath = ss.str();

  auto wideName = Utils::Utf8Decode(ServiceServer::myServiceName);

  const auto handle = CreateService(
      manager, wideName.c_str(), wideName.c_str(), SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
      fullPath.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
  if (!handle) {
    const auto error = GetLastError();
    if (error != ERROR_SERVICE_EXISTS)
      LOG(ERROR) << Utils::FormatError(FormatCreateService, false, error)
                 << '\n';
    return {};
  }

  const auto description = Utils::Utf8Decode(Localizer::Get(TxtAppDescription));
  SERVICE_DESCRIPTIONW sd{};
  sd.lpDescription = const_cast<LPWSTR>(description.c_str());

  if (!ChangeServiceConfig2(handle, SERVICE_CONFIG_DESCRIPTION, &sd))
    LOG(WARNING) << Utils::FormatError(FormatChangeServiceConfig2) << '\n';

  LOG(INFO) << Localizer::Get(TxtServiceInstalled) << '\n';

  Service service = {.handle = handle,
                     .name = ServiceServer::myServiceName,
                     .wideName = std::move(wideName)};

  return service;
}

static bool StartMySvc(const Service &service, const wchar_t *servicePath) {
  std::vector<std::wstring> args;
  args.emplace_back(servicePath);
  for (const auto port : ParsedArguments::instance->ports) {
    args.emplace_back(L"-p");
    args.emplace_back(std::to_wstring(port));
  }
  args.emplace_back(L"-w");
  args.emplace_back(std::to_wstring(ParsedArguments::instance->windowSize));
  args.emplace_back(L"--run-as-service");

  std::vector<const wchar_t *> cArgs;
  for (const auto &arg : args)
    cArgs.emplace_back(arg.c_str());

  if (!StartService(service.handle, static_cast<DWORD>(cArgs.size()),
                    cArgs.data())) {
    LOG(ERROR) << Utils::FormatError(FormatStartService) << '\n';
    return false;
  }

  const auto start = std::chrono::steady_clock::now();
  bool running{};
  bool logStartPending{};
  while (true) {
    const auto status = GetStatus(service);
    if (!status)
      break;

    if (status->dwCurrentState == SERVICE_RUNNING) {
      running = true;
      break;
    }

    if (status->dwCurrentState != SERVICE_START_PENDING)
      break;

    const auto current = std::chrono::steady_clock::now();
    const auto elapsed = current - start;
    if (elapsed >= 2s && !logStartPending) {
      LOG(INFO) << Utils::Format(FormatServiceStartPending,
                                 service.name.c_str())
                << '\n';
      logStartPending = true;
    }
    if (elapsed >= 5s)
      break;

    std::this_thread::sleep_for(500ms);
  }

  if (!running)
    LOG(ERROR) << Utils::Format(FormatServiceNotStart, service.name.c_str())
               << '\n';
  else
    LOG(INFO) << Utils::Format(FormatServiceStarted, service.name.c_str())
              << '\n';

  return running;
}

static std::optional<Service> OpenSvc(SC_HANDLE manager,
                                      const std::string &serviceName) {
  auto wideServiceName = Utils::Utf8Decode(serviceName);
  const auto service =
      OpenService(manager, wideServiceName.c_str(), SERVICE_ALL_ACCESS);

  if (!service) {
    const auto error = GetLastError();
    if (error == ERROR_SERVICE_DOES_NOT_EXIST)
      return {};

    const auto severity =
        serviceName == ServiceServer::myServiceName ? ERROR : WARNING;
    LOG(severity) << FormatError(FormatOpenService, serviceName, error) << '\n';
    return {};
  }

  Service serviceWrapper = {.handle = service,
                            .name = serviceName,
                            .wideName = std::move(wideServiceName)};

  return serviceWrapper;
}

static bool StopSvc(const Service &service) {
  auto status = GetStatus(service);
  if (status && status->dwCurrentState == SERVICE_STOPPED)
    return true;

  SERVICE_STATUS controlStatus{};
  if (!ControlService(service.handle, SERVICE_CONTROL_STOP, &controlStatus)) {
    LOG(ERROR) << FormatError(FormatControlService, service.name) << '\n';
    return false;
  }

  const auto start = std::chrono::steady_clock::now();
  bool stopped{};
  bool logStopPending{};
  while (true) {
    status = GetStatus(service);
    if (!status)
      break;

    if (status->dwCurrentState == SERVICE_STOPPED) {
      stopped = true;
      break;
    }

    if (status->dwCurrentState != SERVICE_STOP_PENDING)
      break;

    const auto current = std::chrono::steady_clock::now();
    const auto elapsed = current - start;
    if (elapsed >= 2s && !logStopPending) {
      LOG(INFO) << Utils::Format(FormatServiceStopPending, service.name.c_str())
                << '\n';
      logStopPending = true;
    }
    if (elapsed >= 5s)
      break;

    std::this_thread::sleep_for(500ms);
  }

  if (!stopped)
    LOG(ERROR) << Utils::Format(FormatServiceNotStop, service.name.c_str())
               << '\n';
  else
    LOG(INFO) << Utils::Format(FormatServiceStopped, service.name.c_str())
              << '\n';

  return stopped;
}

static bool DeleteSvc(const Service &service) {
  auto deleted = DeleteService(service.handle);

  if (!deleted) {
    const auto error = GetLastError();

    if (service.name == ServiceServer::winDivertServiceName) {
      if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
        LOG(NOTICE) << Localizer::Get(TxtRestartNeeded) << '\n';
        return true;
      }
    }

    LOG(ERROR) << FormatError(FormatDeleteService, service.name, error) << '\n';
  } else
    LOG(INFO) << Utils::Format(FormatServiceUninstalled, service.name.c_str())
              << '\n';

  return deleted;
}

int ServiceClient::Install() {
  assert(ParsedArguments::instance);

  auto running = false;
  std::optional<Service> service;

  wchar_t servicePath[MAX_PATH];
  if (!GetModuleFileName(nullptr, servicePath, MAX_PATH)) {
    LOG(ERROR) << Utils::FormatError(FormatGetModuleFileName) << '\n';
    throw std::runtime_error("This should never happen");
  }

  const auto manager = OpenManager();
  if (!manager)
    goto end;

  service = CreateMySvc(*manager, servicePath);
  if (!service)
    goto end;

  running = StartMySvc(*service, servicePath);

end:
  if (service)
    CloseServiceHandle(service->handle);
  if (manager)
    CloseServiceHandle(*manager);

  return running ? EXIT_SUCCESS : EXIT_FAILURE;
}

int ServiceClient::Uninstall() {
  auto myServiceUninstalled = false;
  std::optional<Service> myService;
  std::optional<Service> winDivertService;

  auto manager = OpenManager();
  if (!manager)
    goto end;

  myService = OpenSvc(*manager, ServiceServer::myServiceName);
  if (!myService)
    goto end;

  if (!StopSvc(*myService))
    goto end;

  if (!DeleteSvc(*myService))
    goto end;

  myServiceUninstalled = true;

  winDivertService = OpenSvc(*manager, ServiceServer::winDivertServiceName);
  if (!winDivertService)
    goto end;

  if (!StopSvc(*winDivertService))
    goto end;

  DeleteSvc(*winDivertService);

end:
  if (winDivertService)
    CloseServiceHandle(winDivertService->handle);
  if (myService)
    CloseServiceHandle(myService->handle);
  if (manager)
    CloseServiceHandle(*manager);

  return myServiceUninstalled ? EXIT_SUCCESS : EXIT_FAILURE;
}

int ServiceClient::Run() {
  const auto app = App::Initialize(*ParsedArguments::instance);
  if (!app)
    return EXIT_FAILURE;

  app->Run();

  return EXIT_SUCCESS;
}
