#pragma once

#include "logger.hpp"
#include <memory>
#include <source_location>

namespace spdlog {
class logger;
} // namespace spdlog

class SpdlogLogger : public Logger {
  public:
    SpdlogLogger();

  private:
    std::shared_ptr<spdlog::logger> spdlog_instance_ = nullptr;

    friend class Logger;

  protected:
    void debug_impl_internal(const std::string &msg, const std::source_location &loc) override;
    void info_impl_internal(const std::string &msg, const std::source_location &loc) override;
    void warn_impl_internal(const std::string &msg, const std::source_location &loc) override;
    void error_impl_internal(const std::string &msg, const std::source_location &loc) override;
};
