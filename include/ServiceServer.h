#pragma once

#include <mutex>
#include <Windows.h>

class ServiceServer final {
public:
  static constexpr char myServiceName[] = "TruyCapSteam";
  static constexpr char winDivertServiceName[] = "WinDivert";

  static int Run();

private:
  static ServiceServer instance;

  SERVICE_STATUS status;
  SERVICE_STATUS_HANDLE statusHandle;
  std::mutex statusMutex;

  static void Main(DWORD argc, LPWSTR *argv);

  static void Handle(DWORD control);

  ServiceServer() : status(), statusHandle() {}

  void SetErrorStatus(DWORD error = EXIT_FAILURE);
};
