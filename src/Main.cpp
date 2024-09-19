#include <exception>
#include <print>
#include "aixlog_wrapper.hpp"
#include "ArgumentParser.h"
#include "Logger.h"
#include "ServiceClient.h"
#include "ServiceServer.h"
#include "Utils.h"

int main(int argc, char **argv) {
  try {
    Utils::SetUtf8();
    Localizer::Initialize();
    AixLog::Log::init<SinkConsole>(AixLog::Severity::trace);

    const auto [type, explanatory, _] = ArgumentParser::Parse(argc, argv);

    switch (type) {
    case ParseResultType::Run:
      break;
    case ParseResultType::Install:
      return ServiceClient::Install();
    case ParseResultType::Uninstall:
      return ServiceClient::Uninstall();
    case ParseResultType::ShowHelp:
    case ParseResultType::ShowCompletion:
      std::print("{}", *explanatory);
      return EXIT_SUCCESS;
    case ParseResultType::InvalidArgument:
      LOG(ERROR) << *explanatory << '\n';
      return EXIT_FAILURE;
    }

    return ParsedArguments::instance->runAsService ? ServiceServer::Run()
                                                   : ServiceClient::Run();
  } catch (const std::exception &e) {
    LOG(ERROR) << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
