#include "ArgumentParser.h"
#include <sstream>
#include "args.hxx"
#include "Localizer.h"

static constexpr uint16_t gDefaultPort = 443;
static constexpr uint16_t gDefaultWindowSize = 2;

// If the app's running as a service, just grab the cached parsed values.
// No need to reparse them.
std::unique_ptr<ParsedArguments> ParsedArguments::instance{};

ParseResult ArgumentParser::Parse(int argc, char **argv) {
  args::ArgumentParser parser(Localizer::Get(TxtAppDescription),
                              Localizer::Get(TxtEpilog));
  args::HelpFlag help(parser, "help", Localizer::Get(TxtArgHelp),
                      {'h', "help"});
  args::CompletionFlag completion(parser, {"complete"});
  args::ValueFlagList<uint16_t> argPorts(
      parser, "ports", Localizer::Get(TxtArgPorts), {'p', "port"});
  args::ValueFlag<uint16_t> argWindowSize(
      parser, "window-size", Localizer::Get(TxtArgWindowSize), {'w'});
  args::Group serviceGroup(parser, Localizer::Get(TxtArgService),
                           args::Group::Validators::AtMostOne);
  args::Flag argInstall(serviceGroup, "install-service",
                        Localizer::Get(TxtArgInstall), {"install"});
  args::Flag argUninstall(serviceGroup, "uninstall-service",
                          Localizer::Get(TxtArgUninstall), {"uninstall"});
  args::Flag argRunAsService(parser, "run-as-service", "Run as a service",
                             {"run-as-service"}, args::Options::Hidden);

  ParseResult result;

  try {
    parser.ParseCLI(argc, argv);
  } catch (const args::Completion &e) {
    result.type = ParseResultType::ShowCompletion;
    result.explanatoryString = e.what();
    return result;
  } catch (const args::Help &) {
    std::stringstream ss;
    ss << parser;
    result.type = ParseResultType::ShowHelp;
    result.explanatoryString = ss.str();
    return result;
  } catch (const args::ParseError &e) {
    result.type = ParseResultType::InvalidArgument;
    result.explanatoryString = e.what();
    return result;
  } catch (const args::ValidationError &e) {
    result.type = ParseResultType::InvalidArgument;
    result.explanatoryString = e.what();
    return result;
  }

  if (argUninstall) {
    result.type = ParseResultType::Uninstall;
    return result;
  }

  result.hasArguments = true;
  ParsedArguments::instance = std::make_unique<ParsedArguments>();

  if (argPorts) {
    for (const auto port : *argPorts)
      if (port)
        ParsedArguments::instance->ports.insert(port);
  }
  if (ParsedArguments::instance->ports.empty())
    ParsedArguments::instance->ports.insert(gDefaultPort);

  if (argWindowSize && *argWindowSize)
    ParsedArguments::instance->windowSize = *argWindowSize;
  else
    ParsedArguments::instance->windowSize = gDefaultWindowSize;

  if (argInstall)
    result.type = ParseResultType::Install;
  else {
    result.type = ParseResultType::Run;
    if (argRunAsService)
      ParsedArguments::instance->runAsService = true;
  }

  return result;
}
