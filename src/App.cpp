#include "App.h"
#include <print>
#include <sstream>
#include <winsock.h>
#include "windivert.h"
#include "aixlog_wrapper.hpp"
#include "Localizer.h"
#include "Logger.h"
#include "Utils.h"

static constexpr size_t gPacketSize = 65535;

std::unique_ptr<App> App::Initialize(const ParsedArguments &args) {
  auto app = std::unique_ptr<App>(new App(args));

  app->device =
      WinDivertOpen(app->filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);

  if (app->device == INVALID_HANDLE_VALUE) {
    LOG(ERROR) << Utils::FormatError(FormatWinDivertOpen, true) << '\n';
    return nullptr;
  }

  return app;
}

void App::Run() {
  LogRunningMessage();

  signalHandler.Start(device);

  std::vector<unsigned char> packet(gPacketSize);
  while (true) {
    if (Signal::instance.Signalled()) {
      signalHandler.Wait();
      WinDivertClose(device);

      return;
    }

    packet.clear();

    UINT packetLength{};
    WINDIVERT_ADDRESS address{};
    if (!WinDivertRecv(device, packet.data(), gPacketSize, &packetLength,
                       &address)) {
      const auto error = GetLastError();
      if (error != ERROR_NO_DATA)
        LOG(ERROR) << Utils::FormatError(FormatWinDivertRecv, true, error)
                   << '\n';

      continue;
    }

    PWINDIVERT_IPHDR ipv4Header{};
    PWINDIVERT_IPV6HDR ipv6Header{};
    PWINDIVERT_TCPHDR tcpHeader{};

    if (address.Outbound)
      goto end;

    if (!WinDivertHelperParsePacket(packet.data(), packetLength, &ipv4Header,
                                    &ipv6Header, nullptr, nullptr, nullptr,
                                    &tcpHeader, nullptr, nullptr, nullptr,
                                    nullptr, nullptr))
      goto end;

    if (tcpHeader && (ipv4Header || ipv6Header)) {
      if (tcpHeader->Syn == 1 && tcpHeader->Ack == 1) {
        tcpHeader->Window = htons(windowSize);
        WinDivertHelperCalcChecksums(packet.data(), packetLength, &address, 0);
      }
    }

  end:
    if (!WinDivertSend(device, packet.data(), packetLength, nullptr, &address))
      LOG(ERROR) << Utils::FormatError(FormatWinDivertSend, true) << '\n';
  }
}

App::App(const ParsedArguments &args)
    : runAsService(args.runAsService), ports(args.ports),
      windowSize(args.windowSize), signalHandler(args.runAsService), device() {
  std::stringstream ss;
  ss << "inbound and !impostor and "
        "tcp and tcp.Syn and tcp.Ack and !loopback and ";

  if (ports.size() == 1)
    ss << "tcp.SrcPort == " << *ports.cbegin();
  else {
    ss << '(';
    for (auto i = ports.cbegin(); i != ports.cend(); ++i) {
      ss << "tcp.SrcPort == " << *i;
      if (std::next(i) != ports.cend())
        ss << " or ";
    }
    ss << ')';
  }

  filter = ss.str();
}

void App::LogRunningMessage() const {
  std::stringstream ss;
  ss << Localizer::Get(TxtAppDescription) << '\n';
  ss << Localizer::Get(TxtAppRunning) << '\n';

  ss << '\t' << Localizer::Get(TxtPorts) << ": ";
  for (auto i = ports.cbegin(); i != ports.cend(); ++i) {
    ss << *i;
    if (std::next(i) != ports.cend())
      ss << ", ";
  }
  ss << "\n\t" << Localizer::Get(TxtWindowSize) << ": " << windowSize;

  auto message = ss.str();
  if (runAsService)
    SinkEventLog::ReplaceNewLine(message);

  LOG(INFO, "default") << message << '\n';
}
