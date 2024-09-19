#pragma once

#include <optional>
#include <string>
#include <Windows.h>

enum class SupportedLanguage { Vietnamese, English };

enum LocalizationKey {
  TxtAppDescription,
  TxtEpilog,
  TxtArgHelp,
  TxtArgPorts,
  TxtArgWindowSize,
  TxtArgService,
  TxtArgInstall,
  TxtArgUninstall,
  TxtAppRunning,
  TxtAppStopping,
  TxtServiceInstalled,
  TxtRestartNeeded,
  TxtPort,
  TxtPorts,
  TxtWindowSize,
  FormatGetModuleFileName,
  FormatOpenSCManager,
  FormatCreateService,
  FormatStartService,
  FormatServiceStartPending,
  FormatServiceStarted,
  FormatServiceNotStart,
  FormatDeleteService,
  FormatServiceUninstalled,
  FormatOpenService,
  FormatControlService,
  FormatServiceStopPending,
  FormatServiceStopped,
  FormatServiceNotStop,
  FormatChangeServiceConfig2,
  FormatQueryServiceStatusEx,
  FormatRegisterEventSource,
  FormatStartServiceCtrlDispatcher,
  FormatRegisterServiceCtrlHandler,
  FormatSetConsoleCtrlHandler,
  FormatWinDivertOpen,
  FormatWinDivertRecv,
  FormatWinDivertSend,
  ErrorFileNotFound,
  ErrorAccessDenied,
  ErrorInvalidParameter,
  ErrorInvalidImageHash,
  ErrorDriverFailedPriorUnload,
  ErrorServiceDoesNotExist,
  ErrorDriverBlocked,
  EptSNotRegistered,
  ErrorInsufficientBuffer,
  ErrorNoData,
  ErrorHostUnreachable,
  LocalizationKeySize
};

class Localizer final {
public:
  static SupportedLanguage
  Initialize(std::optional<SupportedLanguage> preferredLanguage = {});

  static std::string Get(LocalizationKey key);

  static std::string GetError(DWORD error);

  Localizer() = delete;
};
