#include "Localizer.h"
#include <assert.h>
#include <map>
#include <stdexcept>
#include "aixlog_wrapper.hpp"
#include "Utils.h"

static auto gLanguage = SupportedLanguage::English;

static const std::map<LocalizationKey, std::string> gEnglishTranslations = {
    {TxtAppDescription,
     R"(TruyCapSteam helps you bypass restrictions placed by some Vietnamese Internet Service Providers that block access to websites like Steam and BBC.
To test if the app works, run it as an administrator and try to visit https://store.steampowered.com/)"},
    {TxtEpilog, "Source code available at: "
                "https://github.com/nguyenminh-phuc/TruyCapSteam"},
    {TxtArgHelp, "Show this help menu"},
    {TxtArgPorts, "HTTPS ports (default: 443)"},
    {TxtArgWindowSize, "TCP window size (default: 2)"},
    {TxtArgService, "Windows service configuration"},
    {TxtArgInstall, "Install TruyCapSteam as a Windows service"},
    {TxtArgUninstall, "Remove TruyCapSteam service"},
    {TxtAppRunning, "TruyCapSteam is running..."},
    {TxtAppStopping, "TruyCapSteam is stopping..."},
    {TxtServiceInstalled,
     "TruyCapSteam is now installed as a Windows service."},
    {TxtRestartNeeded, "Service WinDivert has already been marked for "
                       "deletion. Please restart your PC to finish."},
    {TxtPort, "HTTPS port"},
    {TxtPorts, "HTTPS ports"},
    {TxtWindowSize, "TCP window size"},
    {FormatGetModuleFileName, "Error getting the app path (%lu): %s"},
    {FormatOpenSCManager, "Error opening Service control manager (%lu): %s"},
    {FormatCreateService, "Error creating service (%lu): %s"},
    {FormatStartService, "Error starting service (%lu): %s"},
    {FormatServiceStartPending, "Service %s is starting..."},
    {FormatServiceStarted, "Service %s is now running..."},
    {FormatServiceNotStart, "Error starting service %s."},
    {FormatDeleteService, "Error deleting service %s (%lu): %s"},
    {FormatServiceUninstalled, "%s service has been removed."},
    {FormatOpenService, "Error opening service %s (%lu): %s"},
    {FormatControlService, "Error stopping service %s (%lu): %s"},
    {FormatServiceStopPending, "Service %s is stopping..."},
    {FormatServiceStopped, "Service %s is now stopped."},
    {FormatServiceNotStop, "Error stopping service %s."},
    {FormatChangeServiceConfig2, "Error setting service description (%lu): %s"},
    {FormatQueryServiceStatusEx,
     "Error retrieving service status %s (%lu): %s"},
    {FormatRegisterEventSource, "Error registering event source (%lu): %s"},
    {FormatStartServiceCtrlDispatcher,
     "Error starting app as service (%lu): %s"},
    {FormatRegisterServiceCtrlHandler,
     "Error setting up service handler (%lu): %s"},
    {FormatSetConsoleCtrlHandler, "Error setting Ctrl+C handler (%lu): %s"},
    {FormatWinDivertOpen, "Error opening WinDivert device (%lu): %s"},
    {FormatWinDivertRecv, "Error receiving packet (%lu): %s"},
    {FormatWinDivertSend, "Error sending packet (%lu): %s"},
    {ErrorFileNotFound,
     "WinDivert driver files (WinDivert32.sys or WinDivert64.sys) not found"},
    {ErrorAccessDenied,
     "The application does not have administrator permissions"},
    {ErrorInvalidParameter,
     "There is an issue with the packet filter settings or parameters"},
    {ErrorInvalidImageHash,
     "WinDivert driver files do not have a valid digital signature"},
    {ErrorDriverFailedPriorUnload,
     "An incompatible version of the WinDivert driver is already in use"},
    {ErrorServiceDoesNotExist,
     "The device was opened with the WINDIVERT_FLAG_NO_INSTALL flag and the "
     "WinDivert driver is not already installed"},
    {ErrorDriverBlocked,
     R"(This error can occur if:
1. security software is blocking the WinDivert driver; or
2. you are using a virtual environment that does not support drivers)"},
    {EptSNotRegistered,
     "Error due to Base Filtering Engine service being disabled"},
    {ErrorInsufficientBuffer,
     "The packet being captured is too large for the pPacket buffer"},
    {ErrorNoData, "The device is shut down and the packet queue is empty"},
    {ErrorHostUnreachable,
     "The error happens when an impostor packet is detected and its TTL or "
     "HopLimit value reaches zero"}};

static const std::map<LocalizationKey, std::string> gVietnameseTranslations = {
    {TxtAppDescription,
     R"(TruyCapSteam giúp ta truy cập các trang web bị chặn bởi một số nhà cung cấp dịch vụ Internet ở Việt Nam, như Steam và BBC.
Để kiểm tra chức năng của ứng dụng, hãy chạy phần mềm với quyền Admin và thử truy cập vào https://store.steampowered.com/)"},
    {TxtEpilog,
     "Mã nguồn có sẵn tại: https://github.com/nguyenminh-phuc/TruyCapSteam"},
    {TxtArgHelp, "Hiển thị menu trợ giúp"},
    {TxtArgPorts, "Cổng HTTPS (mặc định: 443)"},
    {TxtArgWindowSize, "Kích thước TCP window (mặc định: 2)"},
    {TxtArgService, "Cấu hình Windows service"},
    {TxtArgInstall, "Cài đặt TruyCapSteam dưới dạng Windows service"},
    {TxtArgUninstall, "Xóa TruyCapSteam service"},
    {TxtAppRunning, "TruyCapSteam đang chạy..."},
    {TxtAppStopping, "TruyCapSteam đang dừng lại..."},
    {TxtServiceInstalled,
     "TruyCapSteam đã được cài đặt dưới dạng Windows service."},
    {TxtRestartNeeded, "Service WinDivert đã được đánh dấu để xóa. "
                       "Vui lòng khởi động lại PC để hoàn tất."},

    {TxtPort, "Cổng HTTPS"},
    {TxtPorts, "Cổng HTTPS"},
    {TxtWindowSize, "Kích thước TCP window"},
    {FormatGetModuleFileName, "Lỗi khi lấy đường dẫn ứng dụng (%lu): %s"},
    {FormatOpenSCManager, "Lỗi khi mở Service control manager (%lu): %s"},
    {FormatCreateService, "Lỗi khi tạo service (%lu): %s"},
    {FormatStartService, "Lỗi khi khởi động service (%lu): %s"},
    {FormatServiceStartPending, "Service %s đang khởi động..."},
    {FormatServiceStarted, "Service %s đã chạy."},
    {FormatServiceNotStart, "Lỗi khi khởi động service %s."},
    {FormatDeleteService, "Lỗi khi xóa service %s (%lu): %s"},
    {FormatServiceUninstalled, "Service %s đã bị xóa."},
    {FormatOpenService, "Lỗi khi mở service %s (%lu): %s"},
    {FormatControlService, "Lỗi khi dừng service %s (%lu): %s"},
    {FormatServiceStopPending, "Service %s đang dừng lại..."},
    {FormatServiceStopped, "Service %s đã dừng lại."},
    {FormatServiceNotStop, "Lỗi khi dừng service %s."},
    {FormatChangeServiceConfig2,
     "Lỗi khi cấu hình service description (%lu): %s"},
    {FormatQueryServiceStatusEx,
     "Lỗi khi lấy trạng thái hiện tại của service %s (%lu): %s"},
    {FormatRegisterEventSource, "Lỗi khi đăng ký nguồn sự kiện (%lu): %s"},
    {FormatStartServiceCtrlDispatcher,
     "Lỗi khi khởi động ứng dụng dưới dạng service Windows (%lu): %s"},
    {FormatRegisterServiceCtrlHandler,
     "Lỗi khi thiết lập service handler (%lu): %s"},
    {FormatSetConsoleCtrlHandler, "Lỗi khi thiết lập Ctrl+C handler (%lu): %s"},
    {FormatWinDivertOpen, "Lỗi khi mở device WinDivert (%lu): %s"},
    {FormatWinDivertRecv, "Lỗi khi nhận gói tin (%lu): %s"},
    {FormatWinDivertSend, "Lỗi khi gửi gói tin (%lu): %s"},
    {ErrorFileNotFound,
     "Không tìm thấy tập tin driver WinDivert32.sys hoặc WinDivert64.sys"},
    {ErrorAccessDenied, "Ứng dụng không có quyền Admin"},
    {ErrorInvalidParameter, "Lỗi khi cài đặt tham số bộ lọc"},
    {ErrorInvalidImageHash,
     "Tập tin driver WinDivert không có chữ ký số hợp lệ"},
    {ErrorDriverFailedPriorUnload,
     "Phiên bản driver WinDivert không tương thích hiện đang được sử dụng"},
    {ErrorServiceDoesNotExist,
     "Device được thiết lập với cờ WINDIVERT_FLAG_NO_INSTALL và driver "
     "WinDivert chưa được cài đặt"},
    {ErrorDriverBlocked,
     R"(Lỗi này có thể do:
1. phần mềm diệt virus chặn driver WinDivert; hoặc
2. bạn đang sử dụng môi trường ảo hóa không hỗ trợ driver)"},
    {EptSNotRegistered, "Lỗi do dịch vụ Base Filtering Engine đã bị tắt"},
    {ErrorInsufficientBuffer, "Gói tin bị bắt lớn hơn bộ đệm pPacket"},
    {ErrorNoData, "Device đã bị tắt và hàng đợi gói tin rỗng"},
    {ErrorHostUnreachable, "Lỗi này xảy ra khi phát hiện gói tin impostor và "
                           "giá trị TTL hoặc HopLimit giảm về 0"}};

SupportedLanguage
Localizer::Initialize(std::optional<SupportedLanguage> preferredLanguage) {
  if (preferredLanguage)
    gLanguage = *preferredLanguage;
  else {
    const auto languageId = GetUserDefaultUILanguage();
    switch (languageId & 0xff) {
    case LANG_VIETNAMESE:
      gLanguage = SupportedLanguage::Vietnamese;
      break;
    case LANG_ENGLISH:
    default:
      gLanguage = SupportedLanguage::English;
      break;
    }
  }

  assert(gEnglishTranslations.size() == LocalizationKeySize);
  assert(gVietnameseTranslations.size() == LocalizationKeySize);

  return gLanguage;
}

std::string Localizer::Get(LocalizationKey key) {
  switch (gLanguage) {
  case SupportedLanguage::English:
    return gEnglishTranslations.at(key);
  case SupportedLanguage::Vietnamese:
    return gVietnameseTranslations.at(key);
  default:
    throw std::runtime_error("This should never happen.");
  }
}

std::string Localizer::GetError(DWORD error) {
  switch (error) {
  case ERROR_FILE_NOT_FOUND:
    return Get(ErrorFileNotFound);
  case ERROR_ACCESS_DENIED:
    return Get(ErrorAccessDenied);
  case ERROR_INVALID_PARAMETER:
    return Get(ErrorInvalidParameter);
  case ERROR_INVALID_IMAGE_HASH:
    return Get(ErrorInvalidImageHash);
  case ERROR_DRIVER_FAILED_PRIOR_UNLOAD:
    return Get(ErrorDriverFailedPriorUnload);
  case ERROR_SERVICE_DOES_NOT_EXIST:
    return Get(ErrorServiceDoesNotExist);
  case ERROR_DRIVER_BLOCKED:
    return Get(ErrorDriverBlocked);
  case EPT_S_NOT_REGISTERED:
    return Get(EptSNotRegistered);
  case ERROR_INSUFFICIENT_BUFFER:
    return Get(ErrorInsufficientBuffer);
  case ERROR_NO_DATA:
    return Get(ErrorNoData);
  case ERROR_HOST_UNREACHABLE:
    return Get(ErrorHostUnreachable);
  default:
    return Utils::GetSystemError(error);
  }
}
