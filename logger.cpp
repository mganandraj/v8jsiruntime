#include <logger.h>

std::mutex Logger::log_m;

void Logger::log(const std::string& s) {
  std::unique_lock<std::mutex> lock(log_m);
  std::cout << s << std::endl;
}

void Logger::log(const char* ch) {
  std::unique_lock<std::mutex> lock(log_m);
  std::cout << ch << std::endl;
}

void Logger::log(const char* prefix, const std::string& s) {
  std::unique_lock<std::mutex> lock(log_m);
  std::cout << std::string(prefix) << " >>>>" << s << std::endl;
}

void Logger::log(const char* prefix, const char* ch) {
  std::unique_lock<std::mutex> lock(log_m);
  std::cout << std::string(prefix) << " >>>>" << ch << std::endl;
}