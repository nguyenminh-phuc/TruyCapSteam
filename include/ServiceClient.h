#pragma once

class ServiceClient final {
public:
  static int Install();

  static int Uninstall();

  static int Run();

  ServiceClient() = delete;
};
