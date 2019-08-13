#pragma once

#include <string>
#include <mutex>
#include <iostream>

class Logger {
public:
  static void log(const std::string& s);
  static void log(const char* ch);

  static void log(const char* prefix, const std::string& s);
  static void log(const char* prefix, const char* ch);

private:
  static std::mutex log_m;
};
